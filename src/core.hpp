#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <numeric>
#include <random>
#include <sstream>
#include <stdexcept>
#include <vector>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif
namespace aa {
constexpr int K=8, N=128, H=48, A=74;
constexpr int supply[K]={6,6,6,6,6,6,1,1};
// Double Agent, Enforcer, Codebreaker, Saboteur, Daredevil, Sentinel, Sidekick, Mole.
constexpr int moves[K][3]={{-1,6,-1},{1,2,3},{0,0,0},{-1,-1,-2},{2,3,0},{0,2,6},{4,4,4},{-3,-3,-3}};
inline void require(bool b,const char* s){if(!b)throw std::runtime_error(s);}
inline double uniform(std::mt19937& r){return (double(r())+0.5)/4294967296.0;}
struct Obs {
 std::array<int,K> hand{},mine{},other{},discards{};
 int gap=7, remainingMine=4,remainingOther=4,deck=30,otherHand=4,shown=-1,phase=0;
 bool operator==(const Obs& b)const {
  return hand==b.hand&&mine==b.mine&&other==b.other&&discards==b.discards&&gap==b.gap&&remainingMine==b.remainingMine&&remainingOther==b.remainingOther&&deck==b.deck&&otherHand==b.otherHand&&shown==b.shown&&phase==b.phase;
 }
};
inline std::vector<int> legal(const Obs& o){
 if(o.phase==1)return {72,73};
 std::vector<int>a; int types=0;for(int n:o.hand)types+=n>0;
 for(int x=0;x<K;x++)if(o.hand[x])for(int y=0;y<K;y++)if(o.hand[y]&&(x!=y||(types==1&&o.hand[x]>=2)))a.push_back(x*8+y);
 if(o.deck>0&&o.remainingMine>0)for(int x=0;x<K;x++)if(o.hand[x])a.push_back(64+x);
 return a;
}
inline std::array<float,N> encode(const Obs&o){
 std::array<float,N>x{};int j=0;
 for(int v:o.hand)x[j++]=v/4.f;
 for(auto p:{o.mine,o.other})for(int v:p){x[j+std::min(3,v)]=1;j+=4;}
 for(int v:o.discards)x[j++]=v/4.f;
 for(int k=0;k<K;k++)x[j++]=std::max(0,supply[k]-o.hand[k]-o.mine[k]-o.other[k]-o.discards[k]-(o.shown==k))/6.f;
 x[j+std::clamp(o.gap,1,13)-1]=1;j+=13;
 x[j+o.shown+1]=1;j+=9;
 x[j++]=o.remainingMine/4.f;x[j++]=o.remainingOther/4.f;
 x[j++]=o.deck/38.f;x[j++]=o.otherHand/4.f;x[j++]=float(o.phase);x[j++]=(7-o.gap)/7.f;
 return x;
}
struct Outcome {
 std::array<bool,2> caught{},codebreakers{},daredevils{};
 bool conflict=false,exhausted=false,distanceTie=false;
 int winner=-1;
};
struct Game {
 std::array<std::array<int,K>,2> hand{},pile{},discard{};
 std::array<int,2> swaps{4,4},progress{0,0};
 std::vector<int> deck;
 int active=0,phase=0,face=-1,hidden=-1,winner=-1,turns=0;
 Game()=default;
 Game(std::mt19937&r,int first){reset(r,first);}
 void reset(std::mt19937&r,int first){
  *this=Game(); active=first;for(int k=0;k<K;k++)for(int n=0;n<supply[k];n++)deck.push_back(k);
  std::shuffle(deck.begin(),deck.end(),r);refill(0);refill(1);
 }
 int countHand(int p)const{return std::accumulate(hand[p].begin(),hand[p].end(),0);}
 void refill(int p){while(countHand(p)<4&&!deck.empty()){hand[p][deck.back()]++;deck.pop_back();}}
 int actor()const{return phase?1-active:active;}
 Obs observe(int p)const{
  Obs o;o.hand=hand[p];o.mine=pile[p];o.other=pile[1-p];o.discards=discard[p];
  o.gap=7+progress[1-p]-progress[p];o.remainingMine=swaps[p];o.remainingOther=swaps[1-p];
  o.deck=int(deck.size());o.otherHand=countHand(1-p);o.shown=phase?face:-1;o.phase=phase;return o;
 }
 Outcome outcome()const{
  Outcome o;
  for(int p=0;p<2;p++){o.caught[p]=progress[p]-progress[1-p]>=7;o.codebreakers[p]=pile[p][2]>=3;o.daredevils[p]=pile[p][4]>=3;}
  bool w0=o.caught[0]||o.codebreakers[0]||o.daredevils[1];
  bool w1=o.caught[1]||o.codebreakers[1]||o.daredevils[0];
  if(w0||w1){o.conflict=w0&&w1;o.winner=o.conflict?active:(w0?0:1);}
  else if(deck.empty()&&countHand(1-active)<2){o.exhausted=true;o.distanceTie=progress[0]==progress[1];o.winner=o.distanceTie?active:(progress[0]>progress[1]?0:1);}
  return o;
 }
 void settle(){winner=outcome().winner;}
 void step(int action){
  require(winner<0,"Game is finished");auto l=legal(observe(actor()));require(std::find(l.begin(),l.end(),action)!=l.end(),"Illegal action");
  if(!phase){
   if(action>=64){int k=action-64;hand[active][k]--;discard[active][k]++;swaps[active]--;refill(active);return;}
   face=action/8;hidden=action%8;hand[active][face]--;hand[active][hidden]--;refill(active);phase=1;return;
  }
  int selected=action==72?face:hidden, left=action==72?hidden:face;
  int cards[2];cards[active]=left;cards[1-active]=selected;
  for(int p=0;p<2;p++){int k=cards[p];pile[p][k]++;progress[p]+=moves[k][std::min(2,pile[p][k]-1)];}
  turns++;settle();phase=0;face=hidden=-1;if(winner<0)active=1-active;
 }
 bool conserved()const{
  for(int k=0;k<K;k++){int n=std::count(deck.begin(),deck.end(),k)+(phase&&face==k)+(phase&&hidden==k);
   for(int p=0;p<2;p++)n+=hand[p][k]+pile[p][k]+discard[p][k];if(n!=supply[k])return false;}
  return true;
 }
};
constexpr int W1=0,B1=N*H,W2=B1+H,B2=W2+A*H,WV=B2+A,BV=WV+H,P=BV+1;
struct Net {
 std::array<float,P>w{};
 void init(std::mt19937&r){
  std::normal_distribution<float>d(0,1);for(int j=0;j<N*H;j++)w[j]=d(r)*std::sqrt(2.f/N);
  for(int j=W2;j<B2;j++)w[j]=d(r)*0.025f;for(int j=WV;j<BV;j++)w[j]=d(r)*0.025f;
 }
 struct Forward{std::array<float,H>h{};std::array<float,A>p{};float v=0;};
 Forward forward(const std::array<float,N>&x,const std::vector<int>&actions)const{
  Forward f;for(int h=0;h<H;h++){float v=w[B1+h];for(int n=0;n<N;n++)v+=w[h*N+n]*x[n];f.h[h]=std::tanh(v);}
  float mx=-1e30f;for(int a:actions){float v=w[B2+a];for(int h=0;h<H;h++)v+=w[W2+a*H+h]*f.h[h];f.p[a]=v;mx=std::max(mx,v);}
  float sum=0;for(int a:actions){f.p[a]=std::exp(f.p[a]-mx);sum+=f.p[a];}for(int a:actions)f.p[a]/=sum;
  float v=w[BV];for(int h=0;h<H;h++)v+=w[WV+h]*f.h[h];f.v=std::tanh(v);return f;
 }
 int act(const Obs&o,std::mt19937&r)const{
  auto l=legal(o);require(!l.empty(),"No legal actions");auto f=forward(encode(o),l);double u=uniform(r);
  for(int a:l){u-=f.p[a];if(u<=0)return a;}return l.back();
 }
};
inline float pairUtility(const Obs&o,int myCard,int otherCard){
 auto mine=o.mine,other=o.other;mine[myCard]++;other[otherCard]++;
 int lead=7-o.gap+moves[myCard][std::min(2,mine[myCard]-1)]-moves[otherCard][std::min(2,other[otherCard]-1)];
 bool win=lead>=7||mine[2]>=3||other[4]>=3;
 bool loss=lead<=-7||other[2]>=3||mine[4]>=3;
 if(win||loss){bool wins=(win&&loss)?o.phase==0:win;return wins?100.f:-100.f;}
 return lead*2.f+(mine[2]-other[2])*1.5f-(mine[4]-other[4])*0.6f+(mine[5]==2?1.f:0.f)-(other[5]==2?1.f:0.f);
}
// Fixed opponent uses only the same observation as the neural policy.
inline int heuristic(const Obs&o,std::mt19937&r){
 auto l=legal(o);std::vector<std::pair<int,float>>s;
 if(o.phase){
  float up=0,down=0,total=0;
  for(int k=0;k<K;k++)if(k!=o.shown){int n=std::max(0,supply[k]-o.hand[k]-o.mine[k]-o.other[k]-o.discards[k]);
   up+=n*pairUtility(o,o.shown,k);down+=n*pairUtility(o,k,o.shown);total+=n;}
  if(total==0)return 72;return up==down?(uniform(r)<.5?72:73):(up>down?72:73);
 }
 float best=-1e30;int chosen=l.front();
 for(int a:l)if(a<64){float v=.7f*std::min(pairUtility(o,a/8,a%8),pairUtility(o,a%8,a/8))+.3f*(pairUtility(o,a/8,a%8)+pairUtility(o,a%8,a/8))*.5f;
  v+=float(uniform(r))*.01f;if(v>best){best=v;chosen=a;}}
 return chosen;
}
struct Sample {Obs o;int action=0,player=0;};
struct Eval {int games=0,randomWins=0,greedyWins=0,championWins=0;};
struct Trainer {
 Net current,champion;
 std::array<float,P>rms{},gradient{};
 std::vector<Net>pool;
 std::mt19937 rng;
 uint64_t games=0,updates=0,championAt=0;int samples=0,batchGames=0;
 Eval last;std::vector<std::array<float,3>>history;
 Trainer(uint32_t seed=49217):rng(seed){current.init(rng);champion=current;pool.push_back(current);}
 void accumulate(const Sample&s,float target){
  auto x=encode(s.o);auto l=legal(s.o);auto f=current.forward(x,l);
  float advantage=target-f.v;std::array<float,H>dh{};
  float entropy=0;for(int a:l)entropy-=f.p[a]*std::log(std::max(1e-9f,f.p[a]));
  for(int a:l){float g=advantage*((a==s.action?1.f:0.f)-f.p[a])-.02f*f.p[a]*(std::log(std::max(1e-9f,f.p[a]))+entropy);
   gradient[B2+a]+=g;for(int h=0;h<H;h++){gradient[W2+a*H+h]+=g*f.h[h];dh[h]+=g*current.w[W2+a*H+h];}}
  float gv=.5f*(target-f.v)*(1-f.v*f.v);gradient[BV]+=gv;
  for(int h=0;h<H;h++){gradient[WV+h]+=gv*f.h[h];dh[h]+=gv*current.w[WV+h];float g=dh[h]*(1-f.h[h]*f.h[h]);gradient[B1+h]+=g;for(int n=0;n<N;n++)gradient[h*N+n]+=g*x[n];}
  samples++;
 }
 void update(){
  if(!samples)return;float norm=0;for(float v:gradient)norm+=(v/samples)*(v/samples);float scale=1/std::max(1.f,std::sqrt(norm));
  for(int i=0;i<P;i++){float g=gradient[i]/samples*scale;rms[i]=.99f*rms[i]+.01f*g*g;current.w[i]+=.00035f*g/(std::sqrt(rms[i])+1e-5f);gradient[i]=0;}
  samples=0;batchGames=0;updates++;
 }
 void episode(){
  Game g(rng,int(rng()%2));int learner=int(rng()%2);double mode=uniform(rng);int old=int(rng()%pool.size());std::vector<Sample>trajectory;
  while(g.winner<0){int p=g.actor();Obs o=g.observe(p);int a;
   if(p==learner||mode<.50){a=current.act(o,rng);trajectory.push_back({o,a,p});}
   else if(mode<.75)a=pool[old].act(o,rng);
   else if(mode<.9)a=heuristic(o,rng);
   else {auto l=legal(o);a=l[rng()%l.size()];}
   g.step(a);
  }
  for(const auto&s:trajectory)accumulate(s,g.winner==s.player?1.f:-1.f);
  games++;batchGames++;if(batchGames>=16)update();
  if(games%2000==0){pool.push_back(current);if(pool.size()>8)pool.erase(pool.begin()+1);}
 }
 Eval evaluate(int n=200)const{
  Eval e;e.games=n;std::mt19937 random(842091); // Independent RNG: evaluation cannot perturb training.
  for(int kind=0;kind<3;kind++)for(int i=0;i<n;i++){
   Game g(random,i%2);int mine=(i/2)%2;
   while(g.winner<0){auto o=g.observe(g.actor());int a;
    if(g.actor()==mine)a=current.act(o,random);
    else if(kind==0){auto l=legal(o);a=l[random()%l.size()];}
    else if(kind==1)a=heuristic(o,random);else a=champion.act(o,random);g.step(a);}
   if(g.winner==mine){if(kind==0)e.randomWins++;else if(kind==1)e.greedyWins++;else e.championWins++;}
  }return e;
 }
 void record(Eval e){
  int previous=last.games?last.greedyWins*e.games/last.games:0;
  last=e;history.push_back({float(games),float(e.randomWins)/e.games,float(e.greedyWins)/e.games});if(history.size()>100)history.erase(history.begin());
  if(e.championWins>e.games*.55&&e.greedyWins>=previous-e.games*.05){champion=current;championAt=games;}
 }
};
inline uint64_t hash(const std::string&s){uint64_t h=14695981039346656037ull;for(unsigned char c:s){h^=c;h*=1099511628211ull;}return h;}
template<class T>void put(std::ostream&o,const T&v){o.write(reinterpret_cast<const char*>(&v),sizeof(v));}
template<class T>void get(std::istream&i,T&v){i.read(reinterpret_cast<char*>(&v),sizeof(v));require(bool(i),"Truncated checkpoint");}
inline std::string pack(const Trainer&t){
 std::ostringstream o(std::ios::binary);uint32_t version=1;put(o,version);put(o,t.current);put(o,t.champion);put(o,t.rms);put(o,t.gradient);put(o,t.games);put(o,t.updates);put(o,t.championAt);put(o,t.samples);put(o,t.batchGames);put(o,t.last);
 uint32_t n=uint32_t(t.pool.size());put(o,n);for(auto &v:t.pool)put(o,v);
 n=uint32_t(t.history.size());put(o,n);for(auto &v:t.history)put(o,v);
 std::ostringstream r;r<<t.rng;auto rs=r.str();n=uint32_t(rs.size());put(o,n);o.write(rs.data(),n);return o.str();
}
inline Trainer unpack(const std::string&s){
 Trainer t;std::istringstream i(s,std::ios::binary);uint32_t v,n;get(i,v);require(v==1,"Incompatible checkpoint version");get(i,t.current);get(i,t.champion);get(i,t.rms);get(i,t.gradient);get(i,t.games);get(i,t.updates);get(i,t.championAt);get(i,t.samples);get(i,t.batchGames);get(i,t.last);
 get(i,n);require(n>=1&&n<=8,"Invalid opponent pool");t.pool.resize(n);for(auto &p:t.pool)get(i,p);
 get(i,n);require(n<=100,"Invalid history");t.history.resize(n);for(auto &p:t.history)get(i,p);
 get(i,n);require(n<20000,"Invalid RNG data");std::string rs(n,' ');i.read(rs.data(),n);require(bool(i),"Truncated RNG");std::istringstream r(rs);r>>t.rng;require(bool(r),"Invalid RNG");
 require(t.batchGames>=0&&t.batchGames<16&&t.samples>=0&&t.samples<2000,"Invalid optimizer state");
 for(float w:t.current.w)require(std::isfinite(w),"Invalid model weights");return t;
}
inline void replaceFile(const std::filesystem::path&from,const std::filesystem::path&to){
#ifdef _WIN32
 require(MoveFileExW(from.c_str(),to.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0,"Cannot commit checkpoint");
#else
 std::filesystem::rename(from,to);
#endif
}
inline void save(const Trainer&t,const std::filesystem::path&path){
 if(!path.parent_path().empty())std::filesystem::create_directories(path.parent_path());auto data=pack(t);auto tmp=path;tmp+=".tmp";auto backup=path;backup+=".bak";
 {std::ofstream o(tmp,std::ios::binary|std::ios::trunc);require(bool(o),"Cannot write save folder");o.write("AAITRN01",8);uint64_t n=data.size(),h=hash(data);put(o,n);put(o,h);o.write(data.data(),data.size());o.flush();require(bool(o),"Checkpoint write failed");}
#ifdef _WIN32
 HANDLE f=CreateFileW(tmp.c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);require(f!=INVALID_HANDLE_VALUE,"Cannot flush save");BOOL ok=FlushFileBuffers(f);CloseHandle(f);require(ok,"Save flush failed");
#endif
 if(std::filesystem::exists(path))std::filesystem::copy_file(path,backup,std::filesystem::copy_options::overwrite_existing);
 replaceFile(tmp,path);
}
inline Trainer load(const std::filesystem::path&p){
 std::ifstream i(p,std::ios::binary);require(bool(i),"Cannot open checkpoint");char magic[8]{};i.read(magic,8);require(bool(i)&&std::string(magic,8)=="AAITRN01","Wrong checkpoint type");uint64_t n,h;get(i,n);get(i,h);require(n<4000000,"Checkpoint too large");std::string s(n,' ');i.read(s.data(),n);require(bool(i)&&hash(s)==h,"Checkpoint damaged (checksum)");return unpack(s);
}
} // namespace aa
