#pragma once
#include "information_state.hpp"
#include <chrono>
#include <limits>
#include <string>

namespace aa {

constexpr int V3_EMBED=32,V3_HIDDEN=32,V3_BELIEF=16,V3_POLICY_INPUT=V3_HIDDEN+V3_BELIEF;

struct V3Config {
 bool recurrent=true;
 bool beliefAuxiliary=true;
 bool beliefConditioned=true;
 int sequenceLength=16;
 int burnIn=4;
 int batchGames=128;
 float learningRate=.00035f;
 float entropyCoefficient=.02f;
 float valueLossWeight=.5f;
 float beliefLossWeight=.25f;
 bool operator==(const V3Config& b)const{
  return recurrent==b.recurrent&&beliefAuxiliary==b.beliefAuxiliary&&beliefConditioned==b.beliefConditioned&&sequenceLength==b.sequenceLength&&burnIn==b.burnIn&&batchGames==b.batchGames&&learningRate==b.learningRate&&entropyCoefficient==b.entropyCoefficient&&valueLossWeight==b.valueLossWeight&&beliefLossWeight==b.beliefLossWeight;
 }
};

enum class V3Block { Encoder, Gru, Belief, Policy, Value };

struct V3Layout {
 size_t encoderW=0,encoderB;
 size_t wz,uz,bz,wr,ur,br,wn,un,bn;
 size_t beliefW,beliefB,policyW,policyB,valueW,valueB,total;
 V3Layout(){
  size_t p=0;encoderW=p;p+=V3_EMBED*HISTORY_FEATURES;encoderB=p;p+=V3_EMBED;
  wz=p;p+=V3_HIDDEN*V3_EMBED;uz=p;p+=V3_HIDDEN*V3_HIDDEN;bz=p;p+=V3_HIDDEN;
  wr=p;p+=V3_HIDDEN*V3_EMBED;ur=p;p+=V3_HIDDEN*V3_HIDDEN;br=p;p+=V3_HIDDEN;
  wn=p;p+=V3_HIDDEN*V3_EMBED;un=p;p+=V3_HIDDEN*V3_HIDDEN;bn=p;p+=V3_HIDDEN;
  beliefW=p;p+=V3_BELIEF*V3_HIDDEN;beliefB=p;p+=V3_BELIEF;
  policyW=p;p+=A*V3_POLICY_INPUT;policyB=p;p+=A;
  valueW=p;p+=V3_HIDDEN;valueB=p;p++;total=p;
 }
 bool inGru(size_t p)const{return p>=wz&&p<beliefW;}
};

inline const V3Layout& v3Layout(){static const V3Layout layout;return layout;}
inline float sigmoid(float x){return 1.f/(1.f+std::exp(-std::clamp(x,-20.f,20.f)));}

struct V3Inference {
 std::array<float,V3_HIDDEN> hidden{};
 std::array<float,K> opponentHand{};
 std::array<float,K> hiddenOffer{};
 std::array<float,A> policy{};
 float value=0;
};

struct V3StepCache {
 std::array<float,HISTORY_FEATURES>x{};
 std::array<float,V3_EMBED>e{};
 std::array<float,V3_HIDDEN>previous{},z{},r{},candidate{},hidden{};
};

inline std::array<bool,K> feasibleTypes(const Obs& o){
 std::array<bool,K> mask{};
 for(int k=0;k<K;k++){
  int remaining=supply[k]-o.hand[k]-o.mine[k]-o.other[k]-o.discards[k]-(o.shown==k);
  mask[k]=remaining>0;
 }
 return mask;
}

template<size_t M>inline std::array<float,M> maskedSoftmax(const std::array<float,M>& logits,const std::array<bool,M>& mask){
 std::array<float,M> output{};float maximum=-std::numeric_limits<float>::infinity();int count=0;
 for(size_t i=0;i<M;i++)if(mask[i]){maximum=std::max(maximum,logits[i]);count++;}
 if(!count)return output;float sum=0;
 for(size_t i=0;i<M;i++)if(mask[i]){output[i]=std::exp(logits[i]-maximum);sum+=output[i];}
 for(float& value:output)value/=sum;return output;
}

struct V3Net {
 std::vector<float> weights;
 V3Net():weights(v3Layout().total){}

 void init(std::mt19937& rng){
  std::normal_distribution<float> normal(0,1);auto& l=v3Layout();
  for(size_t i=0;i<weights.size();i++)weights[i]=normal(rng)*.025f;
  float encoderScale=std::sqrt(2.f/HISTORY_FEATURES);
  for(size_t i=l.encoderW;i<l.encoderB;i++)weights[i]=normal(rng)*encoderScale;
  float recurrentScale=std::sqrt(1.f/V3_HIDDEN);
  for(size_t i=l.wz;i<l.beliefW;i++)weights[i]=normal(rng)*recurrentScale;
  for(int h=0;h<V3_HIDDEN;h++)weights[l.bz+h]=1.f;
 }

 std::vector<V3StepCache> encodeHistory(const PlayerHistory& history,const V3Config& config)const{
  require(!history.events.empty(),"Empty player history");auto& l=v3Layout();
  size_t keep=size_t(std::max(1,config.sequenceLength+config.burnIn));size_t begin=history.events.size()>keep?history.events.size()-keep:0;
  std::vector<V3StepCache> cache;cache.reserve(history.events.size()-begin);
  std::array<float,V3_HIDDEN> previous{};
  for(size_t index=begin;index<history.events.size();index++){
   V3StepCache c;c.x=encodeHistoryEvent(history.events[index]);c.previous=previous;
   for(int e=0;e<V3_EMBED;e++){
    float value=weights[l.encoderB+e];for(int n=0;n<HISTORY_FEATURES;n++)value+=weights[l.encoderW+e*HISTORY_FEATURES+n]*c.x[n];c.e[e]=std::tanh(value);
   }
   if(config.recurrent){
    for(int h=0;h<V3_HIDDEN;h++){
     float z=weights[l.bz+h],r=weights[l.br+h];
     for(int e=0;e<V3_EMBED;e++){z+=weights[l.wz+h*V3_EMBED+e]*c.e[e];r+=weights[l.wr+h*V3_EMBED+e]*c.e[e];}
     for(int q=0;q<V3_HIDDEN;q++){z+=weights[l.uz+h*V3_HIDDEN+q]*previous[q];r+=weights[l.ur+h*V3_HIDDEN+q]*previous[q];}
     c.z[h]=sigmoid(z);c.r[h]=sigmoid(r);
    }
    for(int h=0;h<V3_HIDDEN;h++){
     float value=weights[l.bn+h];for(int e=0;e<V3_EMBED;e++)value+=weights[l.wn+h*V3_EMBED+e]*c.e[e];
     for(int q=0;q<V3_HIDDEN;q++)value+=weights[l.un+h*V3_HIDDEN+q]*(c.r[q]*previous[q]);
     c.candidate[h]=std::tanh(value);c.hidden[h]=(1-c.z[h])*c.candidate[h]+c.z[h]*previous[h];
    }
   }else for(int h=0;h<V3_HIDDEN;h++)c.hidden[h]=c.e[h];
   previous=c.hidden;cache.push_back(c);
  }
  return cache;
 }

 V3Inference infer(const PlayerHistory& history,const V3Config& config)const{
  auto cache=encodeHistory(history,config);V3Inference result;result.hidden=cache.back().hidden;auto& l=v3Layout();
  std::array<float,V3_BELIEF> beliefLogits{};
  for(int b=0;b<V3_BELIEF;b++){float value=weights[l.beliefB+b];for(int h=0;h<V3_HIDDEN;h++)value+=weights[l.beliefW+b*V3_HIDDEN+h]*result.hidden[h];beliefLogits[b]=value;}
  std::array<float,K> handLogits{},offerLogits{};for(int k=0;k<K;k++){handLogits[k]=beliefLogits[k];offerLogits[k]=beliefLogits[K+k];}
  auto feasible=feasibleTypes(history.events.back().observation);result.opponentHand=maskedSoftmax(handLogits,feasible);
  if(history.events.back().observation.phase)result.hiddenOffer=maskedSoftmax(offerLogits,feasible);
  std::array<float,V3_POLICY_INPUT> policyInput{};std::copy(result.hidden.begin(),result.hidden.end(),policyInput.begin());
  if(config.beliefConditioned){std::copy(result.opponentHand.begin(),result.opponentHand.end(),policyInput.begin()+V3_HIDDEN);std::copy(result.hiddenOffer.begin(),result.hiddenOffer.end(),policyInput.begin()+V3_HIDDEN+K);}
  auto actions=legal(history.events.back().observation);std::array<float,A> logits{};std::array<bool,A> mask{};
  for(int action:actions){mask[action]=true;float value=weights[l.policyB+action];for(int q=0;q<V3_POLICY_INPUT;q++)value+=weights[l.policyW+action*V3_POLICY_INPUT+q]*policyInput[q];logits[action]=value;}
  result.policy=maskedSoftmax(logits,mask);float value=weights[l.valueB];for(int h=0;h<V3_HIDDEN;h++)value+=weights[l.valueW+h]*result.hidden[h];result.value=std::tanh(value);return result;
 }

 int act(const PlayerHistory& history,const V3Config& config,std::mt19937& rng,bool greedy=false)const{
  auto actions=legal(history.events.back().observation);auto inference=infer(history,config);
  if(greedy)return *std::max_element(actions.begin(),actions.end(),[&](int a,int b){return inference.policy[a]<inference.policy[b];});
  double draw=uniform(rng);for(int action:actions){draw-=inference.policy[action];if(draw<=0)return action;}return actions.back();
 }
};

struct V3Sample {PlayerHistory history;BeliefTarget belief;int action=0,player=0;float target=0;};
struct V3Metrics {
 uint64_t samples=0,handSamples=0,offerSamples=0;double policyLoss=0,valueLoss=0,beliefLoss=0,handCorrect=0,offerCorrect=0,brier=0;
 std::array<uint64_t,10>calibrationCount{};std::array<double,10>calibrationConfidence{},calibrationAccuracy{};
 double ece()const{double result=0,total=0;for(auto count:calibrationCount)total+=count;if(!total)return 0;for(int i=0;i<10;i++)if(calibrationCount[i])result+=calibrationCount[i]/total*std::abs(calibrationAccuracy[i]/calibrationCount[i]-calibrationConfidence[i]/calibrationCount[i]);return result;}
};
struct V3Eval {int games=0,randomWins=0,heuristicWins=0,baselineWins=0,championWins=0;};
struct V3EvalRecord {
 uint64_t games=0,updates=0,championAt=0,samples=0,handSamples=0,offerSamples=0;
 V3Eval evaluation{};
 std::array<float,4> rates{{-1,-1,-1,-1}};
 double policyLoss=0,valueLoss=0,beliefLoss=0,handAccuracy=0,offerAccuracy=0,brier=0,ece=0;
 bool exactCounts=false;
};

struct V3Trainer {
 V3Config config;V3Net current,champion;std::vector<V3Net> pool;
 std::vector<float> rms,gradient;
 std::mt19937 rng;uint64_t games=0,updates=0,championAt=0;int pendingGames=0,pendingSamples=0;V3Metrics metrics;V3Eval last;std::vector<V3EvalRecord> history;
 explicit V3Trainer(uint32_t seed=49217,V3Config c={}):config(c),rms(v3Layout().total),gradient(v3Layout().total),rng(seed){current.init(rng);champion=current;pool.push_back(current);}

 void recordMetrics(const V3Sample& sample,const V3Inference& output){
  metrics.samples++;metrics.policyLoss-=std::log(std::max(output.policy[sample.action],1e-9f));metrics.valueLoss+=(sample.target-output.value)*(sample.target-output.value);
  auto beliefMetrics=[&](const std::array<float,K>& probability,const std::array<float,K>& target,bool enabled,bool hand){
   if(!enabled)return;double loss=0,brier=0;int predicted=0,actual=0;
   for(int k=0;k<K;k++){loss-=target[k]*std::log(std::max(probability[k],1e-9f));brier+=(probability[k]-target[k])*(probability[k]-target[k]);if(probability[k]>probability[predicted])predicted=k;if(target[k]>target[actual])actual=k;}
   metrics.beliefLoss+=loss;metrics.brier+=brier;double confidence=probability[predicted],accuracy=predicted==actual;int bin=std::min(9,int(confidence*10));metrics.calibrationCount[bin]++;metrics.calibrationConfidence[bin]+=confidence;metrics.calibrationAccuracy[bin]+=accuracy;
   if(hand){metrics.handSamples++;metrics.handCorrect+=accuracy;}else{metrics.offerSamples++;metrics.offerCorrect+=accuracy;}
  };
  if(config.beliefAuxiliary){beliefMetrics(output.opponentHand,sample.belief.opponentHand,sample.belief.hasOpponentHand,true);beliefMetrics(output.hiddenOffer,sample.belief.hiddenOffer,sample.belief.hasHiddenOffer,false);}
 }

 void accumulate(const V3Sample& sample){
  auto cache=current.encodeHistory(sample.history,config);auto output=current.infer(sample.history,config);auto& l=v3Layout();
  std::array<float,V3_POLICY_INPUT> policyInput{};std::copy(output.hidden.begin(),output.hidden.end(),policyInput.begin());
  if(config.beliefConditioned){std::copy(output.opponentHand.begin(),output.opponentHand.end(),policyInput.begin()+V3_HIDDEN);std::copy(output.hiddenOffer.begin(),output.hiddenOffer.end(),policyInput.begin()+V3_HIDDEN+K);}
  std::array<float,A>dPolicy{};float advantage=sample.target-output.value,entropy=0;
  auto actions=legal(sample.history.events.back().observation);
  for(int a:actions)entropy-=output.policy[a]*std::log(std::max(output.policy[a],1e-9f));
  for(int a:actions){
   dPolicy[a]=advantage*((a==sample.action?1.f:0.f)-output.policy[a])-config.entropyCoefficient*output.policy[a]*(std::log(std::max(output.policy[a],1e-9f))+entropy);
   gradient[l.policyB+a]+=dPolicy[a];for(int q=0;q<V3_POLICY_INPUT;q++)gradient[l.policyW+a*V3_POLICY_INPUT+q]+=dPolicy[a]*policyInput[q];
  }
  std::array<float,V3_HIDDEN> dh{};for(int h=0;h<V3_HIDDEN;h++)for(int a:actions)dh[h]+=dPolicy[a]*current.weights[l.policyW+a*V3_POLICY_INPUT+h];
  float dv=config.valueLossWeight*(sample.target-output.value)*(1-output.value*output.value);gradient[l.valueB]+=dv;
  for(int h=0;h<V3_HIDDEN;h++){gradient[l.valueW+h]+=dv*output.hidden[h];dh[h]+=dv*current.weights[l.valueW+h];}

  std::array<float,V3_BELIEF>dBeliefLogits{};
  auto beliefBackprop=[&](const std::array<float,K>& probability,const std::array<float,K>& target,bool enabled,int offset){
   if(!enabled)return;for(int k=0;k<K;k++)dBeliefLogits[offset+k]+=config.beliefLossWeight*(target[k]-probability[k]);
  };
  if(config.beliefAuxiliary){beliefBackprop(output.opponentHand,sample.belief.opponentHand,sample.belief.hasOpponentHand,0);beliefBackprop(output.hiddenOffer,sample.belief.hiddenOffer,sample.belief.hasHiddenOffer,K);}
  if(config.beliefConditioned){
   for(int group=0;group<2;group++){
    auto probability=group?output.hiddenOffer:output.opponentHand;std::array<float,K>dProbability{};
    for(int k=0;k<K;k++)for(int a:actions)dProbability[k]+=dPolicy[a]*current.weights[l.policyW+a*V3_POLICY_INPUT+V3_HIDDEN+group*K+k];
    float dot=0;for(int k=0;k<K;k++)dot+=probability[k]*dProbability[k];for(int k=0;k<K;k++)dBeliefLogits[group*K+k]+=probability[k]*(dProbability[k]-dot);
   }
  }
  for(int b=0;b<V3_BELIEF;b++){gradient[l.beliefB+b]+=dBeliefLogits[b];for(int h=0;h<V3_HIDDEN;h++){gradient[l.beliefW+b*V3_HIDDEN+h]+=dBeliefLogits[b]*output.hidden[h];dh[h]+=dBeliefLogits[b]*current.weights[l.beliefW+b*V3_HIDDEN+h];}}

  int trainSteps=std::min<int>(config.sequenceLength,cache.size()),stop=int(cache.size())-trainSteps;
  for(int index=int(cache.size())-1;index>=stop;index--){auto& c=cache[index];std::array<float,V3_EMBED>de{};std::array<float,V3_HIDDEN>dhPrevious{};
   if(config.recurrent){
    std::array<float,V3_HIDDEN>daZ{},daR{},daN{},dq{};
    for(int h=0;h<V3_HIDDEN;h++){daN[h]=dh[h]*(1-c.z[h])*(1-c.candidate[h]*c.candidate[h]);daZ[h]=dh[h]*(c.previous[h]-c.candidate[h])*c.z[h]*(1-c.z[h]);dhPrevious[h]+=dh[h]*c.z[h];}
    for(int h=0;h<V3_HIDDEN;h++){gradient[l.bn+h]+=daN[h];for(int e=0;e<V3_EMBED;e++){gradient[l.wn+h*V3_EMBED+e]+=daN[h]*c.e[e];de[e]+=daN[h]*current.weights[l.wn+h*V3_EMBED+e];}for(int q=0;q<V3_HIDDEN;q++){gradient[l.un+h*V3_HIDDEN+q]+=daN[h]*(c.r[q]*c.previous[q]);dq[q]+=daN[h]*current.weights[l.un+h*V3_HIDDEN+q];}}
    for(int q=0;q<V3_HIDDEN;q++){daR[q]=dq[q]*c.previous[q]*c.r[q]*(1-c.r[q]);dhPrevious[q]+=dq[q]*c.r[q];}
    for(int h=0;h<V3_HIDDEN;h++)for(auto gate:{0,1}){
     float da=gate?daR[h]:daZ[h];size_t wx=gate?l.wr:l.wz,uh=gate?l.ur:l.uz,bias=gate?l.br:l.bz;gradient[bias+h]+=da;
     for(int e=0;e<V3_EMBED;e++){gradient[wx+h*V3_EMBED+e]+=da*c.e[e];de[e]+=da*current.weights[wx+h*V3_EMBED+e];}
     for(int q=0;q<V3_HIDDEN;q++){gradient[uh+h*V3_HIDDEN+q]+=da*c.previous[q];dhPrevious[q]+=da*current.weights[uh+h*V3_HIDDEN+q];}
    }
   }else for(int e=0;e<V3_EMBED;e++)de[e]=dh[e];
   for(int e=0;e<V3_EMBED;e++){float da=de[e]*(1-c.e[e]*c.e[e]);gradient[l.encoderB+e]+=da;for(int n=0;n<HISTORY_FEATURES;n++)gradient[l.encoderW+e*HISTORY_FEATURES+n]+=da*c.x[n];}
   dh=dhPrevious;
  }
  recordMetrics(sample,output);pendingSamples++;
 }

 void update(){
  if(!pendingSamples)return;float norm=0;for(float value:gradient){float g=value/pendingSamples;norm+=g*g;}float scale=1/std::max(1.f,std::sqrt(norm));
  for(size_t i=0;i<gradient.size();i++){float g=gradient[i]/pendingSamples*scale;rms[i]=.99f*rms[i]+.01f*g*g;current.weights[i]+=config.learningRate*g/(std::sqrt(rms[i])+1e-5f);gradient[i]=0;}
  pendingSamples=0;pendingGames=0;updates++;
 }

 void episode(){
  Game game(rng,int(rng()%2));InformationState information(game);std::vector<V3Sample> trajectory;int learner=int(rng()%2);double mode=uniform(rng);int old=int(rng()%pool.size());
  while(game.winner<0){int player=game.actor();auto playerHistory=information.view(player);auto belief=makeBeliefTarget(game,player);int action;
   if(player==learner||mode<.50){action=current.act(playerHistory,config,rng);trajectory.push_back({std::move(playerHistory),belief,action,player,0});}
   else if(mode<.75)action=pool[old].act(playerHistory,config,rng);else if(mode<.9)action=heuristic(game.observe(player),rng);else{auto actions=legal(game.observe(player));action=actions[rng()%actions.size()];}
   step(game,information,action);}
  for(auto& sample:trajectory){sample.target=game.winner==sample.player?1.f:-1.f;accumulate(sample);}games++;pendingGames++;if(pendingGames>=config.batchGames)update();
  if(games%2000==0){pool.push_back(current);if(pool.size()>8)pool.erase(pool.begin()+1);}
 }

 V3Eval evaluate(int n,const Net* baseline=nullptr)const{
  V3Eval evaluation;evaluation.games=n;std::mt19937 random(842091);
  for(int kind=0;kind<4;kind++)for(int gameIndex=0;gameIndex<n;gameIndex++){
   Game game(random,gameIndex%2);InformationState information(game);int mine=(gameIndex/2)%2;
   while(game.winner<0){int player=game.actor();int action;
    if(player==mine)action=current.act(information.view(player),config,random,true);
    else if(kind==0){auto actions=legal(game.observe(player));action=actions[random()%actions.size()];}
    else if(kind==1)action=heuristic(game.observe(player),random);
    else if(kind==2&&baseline){auto observation=game.observe(player);auto actions=legal(observation);auto forward=baseline->forward(encode(observation),actions);action=*std::max_element(actions.begin(),actions.end(),[&](int a,int b){return forward.p[a]<forward.p[b];});}
    else action=champion.act(information.view(player),config,random,true);
    step(game,information,action);
   }
   if(game.winner==mine){if(kind==0)evaluation.randomWins++;else if(kind==1)evaluation.heuristicWins++;else if(kind==2)evaluation.baselineWins++;else evaluation.championWins++;}
  }
  return evaluation;
 }

 void record(V3Eval evaluation){
  require(evaluation.games>0,"Evaluation must contain games");int previous=last.games?last.heuristicWins*evaluation.games/last.games:0;last=evaluation;V3EvalRecord point;point.games=games;point.updates=updates;point.championAt=championAt;point.evaluation=evaluation;point.rates={float(evaluation.randomWins)/evaluation.games,float(evaluation.heuristicWins)/evaluation.games,float(evaluation.baselineWins)/evaluation.games,float(evaluation.championWins)/evaluation.games};point.samples=metrics.samples;point.handSamples=metrics.handSamples;point.offerSamples=metrics.offerSamples;point.policyLoss=metrics.policyLoss/std::max<uint64_t>(1,metrics.samples);point.valueLoss=metrics.valueLoss/std::max<uint64_t>(1,metrics.samples);point.beliefLoss=metrics.beliefLoss/std::max<uint64_t>(1,metrics.handSamples+metrics.offerSamples);point.handAccuracy=metrics.handCorrect/std::max<uint64_t>(1,metrics.handSamples);point.offerAccuracy=metrics.offerCorrect/std::max<uint64_t>(1,metrics.offerSamples);point.brier=metrics.brier/std::max<uint64_t>(1,metrics.handSamples+metrics.offerSamples);point.ece=metrics.ece();point.exactCounts=true;history.push_back(point);if(history.size()>100)history.erase(history.begin());
  if(evaluation.championWins>evaluation.games*.55&&evaluation.heuristicWins>=previous-evaluation.games*.05){champion=current;championAt=games;}
 }
};

enum class CheckpointKind { Unknown, BaselineV2, RecurrentBeliefV3 };
inline CheckpointKind checkpointKind(const std::filesystem::path& path){
 std::ifstream input(path,std::ios::binary);char magic[8]{};input.read(magic,8);if(!input)return CheckpointKind::Unknown;
 std::string value(magic,8);if(value=="AAITRN01")return CheckpointKind::BaselineV2;if(value=="AAIV3C01")return CheckpointKind::RecurrentBeliefV3;return CheckpointKind::Unknown;
}

template<class T>inline void v3Put(std::ostream& out,const T& value){out.write(reinterpret_cast<const char*>(&value),sizeof(value));}
template<class T>inline void v3Get(std::istream& in,T& value){in.read(reinterpret_cast<char*>(&value),sizeof(value));require(bool(in),"Truncated v3 checkpoint");}
inline void putVector(std::ostream& out,const std::vector<float>& values){uint64_t n=values.size();v3Put(out,n);out.write(reinterpret_cast<const char*>(values.data()),std::streamsize(n*sizeof(float)));}
inline void getVector(std::istream& in,std::vector<float>& values){uint64_t n=0;v3Get(in,n);require(n==v3Layout().total,"V3 architecture size mismatch");values.resize(size_t(n));in.read(reinterpret_cast<char*>(values.data()),std::streamsize(n*sizeof(float)));require(bool(in),"Truncated v3 weights");}

inline void putConfig(std::ostream& out,const V3Config& c){uint8_t recurrent=c.recurrent,auxiliary=c.beliefAuxiliary,conditioned=c.beliefConditioned;v3Put(out,recurrent);v3Put(out,auxiliary);v3Put(out,conditioned);v3Put(out,c.sequenceLength);v3Put(out,c.burnIn);v3Put(out,c.batchGames);v3Put(out,c.learningRate);v3Put(out,c.entropyCoefficient);v3Put(out,c.valueLossWeight);v3Put(out,c.beliefLossWeight);}
inline V3Config getConfig(std::istream& in){V3Config c;uint8_t recurrent=0,auxiliary=0,conditioned=0;v3Get(in,recurrent);v3Get(in,auxiliary);v3Get(in,conditioned);c.recurrent=recurrent;c.beliefAuxiliary=auxiliary;c.beliefConditioned=conditioned;v3Get(in,c.sequenceLength);v3Get(in,c.burnIn);v3Get(in,c.batchGames);v3Get(in,c.learningRate);v3Get(in,c.entropyCoefficient);v3Get(in,c.valueLossWeight);v3Get(in,c.beliefLossWeight);require(c.sequenceLength>0&&c.sequenceLength<=128&&c.burnIn>=0&&c.burnIn<=128&&c.batchGames>0&&c.batchGames<=4096,"Invalid v3 config");return c;}
inline void putMetrics(std::ostream& out,const V3Metrics& m){v3Put(out,m.samples);v3Put(out,m.handSamples);v3Put(out,m.offerSamples);v3Put(out,m.policyLoss);v3Put(out,m.valueLoss);v3Put(out,m.beliefLoss);v3Put(out,m.handCorrect);v3Put(out,m.offerCorrect);v3Put(out,m.brier);v3Put(out,m.calibrationCount);v3Put(out,m.calibrationConfidence);v3Put(out,m.calibrationAccuracy);}
inline void getMetrics(std::istream& in,V3Metrics& m){v3Get(in,m.samples);v3Get(in,m.handSamples);v3Get(in,m.offerSamples);v3Get(in,m.policyLoss);v3Get(in,m.valueLoss);v3Get(in,m.beliefLoss);v3Get(in,m.handCorrect);v3Get(in,m.offerCorrect);v3Get(in,m.brier);v3Get(in,m.calibrationCount);v3Get(in,m.calibrationConfidence);v3Get(in,m.calibrationAccuracy);}
inline void putEvalRecord(std::ostream& out,const V3EvalRecord& p){v3Put(out,p.games);v3Put(out,p.updates);v3Put(out,p.championAt);v3Put(out,p.samples);v3Put(out,p.handSamples);v3Put(out,p.offerSamples);v3Put(out,p.evaluation);v3Put(out,p.rates);v3Put(out,p.policyLoss);v3Put(out,p.valueLoss);v3Put(out,p.beliefLoss);v3Put(out,p.handAccuracy);v3Put(out,p.offerAccuracy);v3Put(out,p.brier);v3Put(out,p.ece);uint8_t exact=p.exactCounts;v3Put(out,exact);}
inline V3EvalRecord getEvalRecord(std::istream& in){V3EvalRecord p;v3Get(in,p.games);v3Get(in,p.updates);v3Get(in,p.championAt);v3Get(in,p.samples);v3Get(in,p.handSamples);v3Get(in,p.offerSamples);v3Get(in,p.evaluation);v3Get(in,p.rates);v3Get(in,p.policyLoss);v3Get(in,p.valueLoss);v3Get(in,p.beliefLoss);v3Get(in,p.handAccuracy);v3Get(in,p.offerAccuracy);v3Get(in,p.brier);v3Get(in,p.ece);uint8_t exact=0;v3Get(in,exact);p.exactCounts=exact!=0;return p;}
inline void putNet(std::ostream& out,const V3Net& net){putVector(out,net.weights);}
inline V3Net getNet(std::istream& in){V3Net net;getVector(in,net.weights);return net;}

inline std::string packV3(const V3Trainer& trainer,uint32_t version=31){
 require(version==3||version==31,"Unsupported v3 pack format");std::ostringstream out(std::ios::binary);uint32_t modelType=2,input=HISTORY_FEATURES,hidden=V3_HIDDEN,belief=V3_BELIEF;v3Put(out,version);v3Put(out,modelType);v3Put(out,input);v3Put(out,hidden);v3Put(out,belief);putConfig(out,trainer.config);putNet(out,trainer.current);putNet(out,trainer.champion);uint32_t poolSize=uint32_t(trainer.pool.size());v3Put(out,poolSize);for(auto& net:trainer.pool)putNet(out,net);putVector(out,trainer.rms);putVector(out,trainer.gradient);v3Put(out,trainer.games);v3Put(out,trainer.updates);v3Put(out,trainer.championAt);v3Put(out,trainer.pendingGames);v3Put(out,trainer.pendingSamples);putMetrics(out,trainer.metrics);v3Put(out,trainer.last);uint32_t historySize=uint32_t(trainer.history.size());v3Put(out,historySize);for(auto& point:trainer.history)if(version==31)putEvalRecord(out,point);else{std::array<float,3> legacy{float(point.games),point.rates[0],point.rates[1]};v3Put(out,legacy);}std::ostringstream random;random<<trainer.rng;auto state=random.str();uint32_t n=uint32_t(state.size());v3Put(out,n);out.write(state.data(),n);return out.str();
}

inline V3Trainer unpackV3(const std::string& data){
 std::istringstream in(data,std::ios::binary);uint32_t version=0,modelType=0,input=0,hidden=0,belief=0;v3Get(in,version);v3Get(in,modelType);v3Get(in,input);v3Get(in,hidden);v3Get(in,belief);require((version==3||version==31)&&modelType==2,"Incompatible v3 checkpoint model");require(input==HISTORY_FEATURES&&hidden==V3_HIDDEN&&belief==V3_BELIEF,"Incompatible v3 architecture");V3Config config=getConfig(in);V3Trainer trainer(49217,config);trainer.current=getNet(in);trainer.champion=getNet(in);uint32_t poolSize=0;v3Get(in,poolSize);require(poolSize>=1&&poolSize<=8,"Invalid v3 opponent pool");trainer.pool.resize(poolSize);for(auto& net:trainer.pool)net=getNet(in);getVector(in,trainer.rms);getVector(in,trainer.gradient);v3Get(in,trainer.games);v3Get(in,trainer.updates);v3Get(in,trainer.championAt);v3Get(in,trainer.pendingGames);v3Get(in,trainer.pendingSamples);getMetrics(in,trainer.metrics);v3Get(in,trainer.last);uint32_t historySize=0;v3Get(in,historySize);require(historySize<=100,"Invalid v3 evaluation history");trainer.history.reserve(historySize);for(uint32_t index=0;index<historySize;index++)if(version==31)trainer.history.push_back(getEvalRecord(in));else{std::array<float,3> legacy{};v3Get(in,legacy);V3EvalRecord point;point.games=uint64_t(std::max(0.f,legacy[0]));point.rates={legacy[1],legacy[2],-1,-1};trainer.history.push_back(point);}uint32_t n=0;v3Get(in,n);require(n<20000,"Invalid v3 RNG state");std::string state(n,' ');in.read(state.data(),n);require(bool(in),"Truncated v3 RNG state");std::istringstream random(state);random>>trainer.rng;require(bool(random),"Invalid v3 RNG state");for(float value:trainer.current.weights)require(std::isfinite(value),"Invalid v3 weights");return trainer;
}

inline void saveV3(const V3Trainer& trainer,const std::filesystem::path& path){
 if(!path.parent_path().empty())std::filesystem::create_directories(path.parent_path());auto data=packV3(trainer);auto temporary=path;temporary+=".tmp";auto backup=path;backup+=".bak";
 {std::ofstream out(temporary,std::ios::binary|std::ios::trunc);require(bool(out),"Cannot write v3 checkpoint");out.write("AAIV3C01",8);uint64_t size=data.size(),checksum=hash(data);v3Put(out,size);v3Put(out,checksum);out.write(data.data(),std::streamsize(data.size()));out.flush();require(bool(out),"V3 checkpoint write failed");}
#if defined(_WIN32) && !defined(AA_PREVIEW)
 HANDLE file=CreateFileW(temporary.c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);require(file!=INVALID_HANDLE_VALUE,"Cannot flush v3 checkpoint");BOOL ok=FlushFileBuffers(file);CloseHandle(file);require(ok,"V3 checkpoint flush failed");
#endif
 if(std::filesystem::exists(path))std::filesystem::copy_file(path,backup,std::filesystem::copy_options::overwrite_existing);replaceFile(temporary,path);
}

inline V3Trainer loadV3(const std::filesystem::path& path){
 std::ifstream in(path,std::ios::binary);require(bool(in),"Cannot open v3 checkpoint");char magic[8]{};in.read(magic,8);require(bool(in)&&std::string(magic,8)=="AAIV3C01","Not a v3 checkpoint");uint64_t size=0,checksum=0;v3Get(in,size);v3Get(in,checksum);require(size<64000000,"V3 checkpoint too large");std::string data(size,' ');in.read(data.data(),std::streamsize(size));require(bool(in)&&hash(data)==checksum,"V3 checkpoint damaged (checksum)");return unpackV3(data);
}

} // namespace aa
