#include "../src/v3_model.hpp"
#include <chrono>
#include <iomanip>
#include <iostream>

using namespace aa;

static std::pair<double,double> wilson(int wins,int games){
 if(!games)return {0,0};double p=double(wins)/games,z=1.959963984540054,den=1+z*z/games,center=(p+z*z/(2*games))/den,margin=z*std::sqrt((p*(1-p)+z*z/(4*games))/games)/den;return {center-margin,center+margin};
}

static void result(const char* name,int wins,int games){auto ci=wilson(wins,games);std::cout<<name<<"="<<wins<<"/"<<games<<" ("<<100.0*wins/games<<"%, 95% CI "<<100*ci.first<<".."<<100*ci.second<<"%) ";}

int main(int argc,char** argv){
 try{
  int trainGames=argc>1?std::stoi(argv[1]):64,evaluationGames=argc>2?std::stoi(argv[2]):100;std::filesystem::path baselinePath=argc>3?argv[3]:"models/pretrained-17m.bin",output=argc>4?argv[4]:"v3-benchmark.bin";
  Net baseline=loadBaselineNet(baselinePath);std::cout<<std::fixed<<std::setprecision(3);
  struct Variant{const char*name;V3Config config;};V3Config recurrentOnly;recurrentOnly.beliefAuxiliary=false;recurrentOnly.beliefConditioned=false;V3Config beliefOnly;beliefOnly.recurrent=false;beliefOnly.beliefConditioned=false;V3Config recurrentBelief;recurrentBelief.beliefConditioned=false;
  std::vector<Variant>variants={{"recurrent-only",recurrentOnly},{"belief-only",beliefOnly},{"recurrent-belief",recurrentBelief},{"belief-conditioned",V3Config{}}};
  for(size_t variant=0;variant<variants.size();variant++){
   V3Trainer trainer(49217,variants[variant].config);auto trainStart=std::chrono::steady_clock::now();for(int i=0;i<trainGames;i++)trainer.episode();if(trainer.pendingSamples)trainer.update();double trainSeconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-trainStart).count();
   std::mt19937 random(123);Game game(random,0);InformationState information(game);auto latencyStart=std::chrono::steady_clock::now();for(int i=0;i<1000;i++)trainer.current.infer(information.view(0),trainer.config);double latencyUs=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-latencyStart).count()/1000;
   auto evaluation=trainer.evaluate(evaluationGames,&baseline);std::cout<<variants[variant].name<<": train_games="<<trainGames<<" train_s="<<trainSeconds<<" games_per_s="<<trainGames/trainSeconds<<" inference_us="<<latencyUs<<" ";result("random",evaluation.randomWins,evaluation.games);result("heuristic",evaluation.heuristicWins,evaluation.games);result("v2",evaluation.baselineWins,evaluation.games);result("champion",evaluation.championWins,evaluation.games);
   double beliefDen=std::max<uint64_t>(1,trainer.metrics.handSamples+trainer.metrics.offerSamples);std::cout<<"belief_ce="<<trainer.metrics.beliefLoss/beliefDen<<" brier="<<trainer.metrics.brier/beliefDen<<" ece="<<trainer.metrics.ece()<<"\n";
   if(variant+1==variants.size()){
    saveV3(trainer,output);auto size=std::filesystem::file_size(output);auto loadStart=std::chrono::steady_clock::now();auto restored=loadV3(output);double loadMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-loadStart).count();require(restored.games==trainer.games,"Benchmark checkpoint resume mismatch");std::cout<<"checkpoint_bytes="<<size<<" load_ms="<<loadMs<<"\n";
   }
  }
  return 0;
 }catch(const std::exception& error){std::cerr<<"FAIL: "<<error.what()<<"\n";return 1;}
}
