#include "../src/core.hpp"
#include <chrono>
#include <iostream>
using namespace aa;
void tests(){
 // Reproduce the user's screenshot, including the two final recruit steps.
 Game screenshot;screenshot.active=0;screenshot.turns=6;screenshot.progress={1,4};
 screenshot.pile[0]={1,1,1,1,1,1,0,0};screenshot.pile[1]={3,0,2,0,0,1,0,0};
 screenshot.hand[0][3]=1;screenshot.hand[0][4]=1;screenshot.hand[0][1]=2;
 screenshot.hand[1][5]=1;screenshot.hand[1][4]=1;screenshot.hand[1][3]=1;screenshot.hand[1][2]=1;
 screenshot.discard[0][0]=screenshot.discard[1][0]=1;screenshot.swaps={3,3};
 for(int k=0;k<K;k++){int remaining=supply[k];for(int p=0;p<2;p++)remaining-=screenshot.pile[p][k]+screenshot.hand[p][k]+screenshot.discard[p][k];for(int n=0;n<remaining;n++)screenshot.deck.push_back(k);}
 require(screenshot.conserved(),"Screenshot initial inventory");
 screenshot.step(3*8+4);screenshot.step(73); // AI receives first Daredevil.
 screenshot.step(5*8+4);screenshot.step(72); // Human Sentinel; AI second Daredevil.
 require(screenshot.pile[0][4]==1&&screenshot.pile[1][4]==2,"Screenshot card counts");
 require(screenshot.progress[0]==2&&screenshot.progress[1]==9&&screenshot.winner==1,"Screenshot capture outcome");
 require(screenshot.deck.size()==12&&screenshot.conserved(),"Screenshot final inventory");
 require(screenshot.outcome().caught[1]&&!screenshot.outcome().daredevils[1],"Screenshot reason");
 // Separately enumerate the rulebook conditions, not the engine's helper.
 int cases=0;
 for(int active=0;active<2;active++)for(int lead=-14;lead<=14;lead++)for(int c0=0;c0<4;c0++)for(int c1=0;c1<4;c1++)for(int d0=0;d0<4;d0++)for(int d1=0;d1<4;d1++){
  Game g;g.active=active;g.progress={lead,0};g.deck={0};g.hand[0][0]=2;g.hand[1][1]=2;
  g.pile[0][2]=c0;g.pile[1][2]=c1;g.pile[0][4]=d0;g.pile[1][4]=d1;
  bool win[2]={lead>=7||c0==3,lead<=-7||c1==3},lose[2]={d0==3,d1==3};
  int expected=-1;
  if((win[0]&&win[1])||(lose[0]&&lose[1])||(win[0]&&lose[0])||(win[1]&&lose[1]))expected=active;
  else if(win[0]||lose[1])expected=0;else if(win[1]||lose[0])expected=1;
  g.settle();require(g.winner==expected,"Independent rulebook comparison");
  auto mirrored=g;std::swap(mirrored.progress[0],mirrored.progress[1]);std::swap(mirrored.pile[0],mirrored.pile[1]);mirrored.active=1-active;mirrored.settle();
  require(mirrored.winner==(expected<0?-1:1-expected),"Player swap symmetry");cases++;
 }
 for(int active=0;active<2;active++){
  Game aiLoses;aiLoses.active=active;aiLoses.hand[0][1]=2;aiLoses.hand[1][1]=2;aiLoses.pile[1][4]=3;aiLoses.deck={0};aiLoses.settle();
  require(aiLoses.winner==0&&aiLoses.outcome().daredevils[1],"AI third Daredevil must lose absent conflict");
 }
 std::cout<<"PASS: screenshot reproduction; "<<cases<<" rulebook outcome combinations and mirrored positions; AI third-Daredevil loss.\n";
 std::mt19937 rng(7123);int actions=0;
 for(int i=0;i<5000;i++){
  Game g(rng,i%2);require(g.deck.size()==30&&g.conserved(),"Initial deal");
  while(g.winner<0){auto l=legal(g.observe(g.actor()));require(!l.empty(),"Reachable dead end");g.step(l[rng()%l.size()]);require(g.conserved(),"Card conservation");require(++actions>0&&g.turns<=19,"Game termination bound");}
 }
 // Both meeples move before checking capture: one can escape in the same round.
 Game g;g.active=0;g.progress={5,0};g.hand[0][1]=2;g.hand[1][1]=2;g.deck={1};g.settle();require(g.winner<0,"Premature capture");
 g.progress={7,1};g.settle();require(g.winner<0,"Capture cancellation");
 g.progress={8,1};g.settle();require(g.winner==0,"Capture threshold");
 for(int active=0;active<2;active++){
  g=Game();g.active=active;g.pile[0][2]=3;g.pile[1][2]=3;g.settle();require(g.winner==active,"Both win tie");
  g=Game();g.active=active;g.pile[0][2]=3;g.pile[0][4]=3;g.settle();require(g.winner==active,"Win plus loss tie");
  g=Game();g.active=active;g.pile[0][4]=3;g.pile[1][4]=3;g.settle();require(g.winner==active,"Both lose tie");
  g=Game();g.active=active;g.settle();require(g.winner==active,"Exhaustion tie");
 }
 g=Game();g.hand[0][2]=2;g.hand[1][1]=2;g.pile[0][2]=2;g.active=0;g.step(2*8+2);g.step(72);require(g.winner==0,"Third codebreaker");
 g=Game();g.hand[0][4]=2;g.hand[1][1]=2;g.pile[0][4]=2;g.active=0;g.step(4*8+4);g.step(72);require(g.winner==1,"Third daredevil");
 g=Game();g.hand[0][0]=2;g.hand[1][1]=2;g.pile[0][0]=3;g.step(0);g.step(72);require(g.progress[0]==-1,"Fourth card movement");
 Obs o;o.hand[0]=4;auto l=legal(o);require(std::find(l.begin(),l.end(),0)!=l.end(),"Identical-only exception");o.hand[1]=1;l=legal(o);require(std::find(l.begin(),l.end(),0)==l.end(),"Different-name requirement");
 o.deck=0;l=legal(o);for(int a:l)require(a<64,"No discard on empty deck");o.deck=5;o.remainingMine=0;l=legal(o);for(int a:l)require(a<64,"Swap limit");
 // The policy API cannot see opponent cards, deck order, or the face-down offer.
 g=Game(rng,0);g.step(legal(g.observe(0))[0]);auto before=g.observe(1);auto features=encode(before);
 g.hidden=(g.hidden+1)%8;std::reverse(g.deck.begin(),g.deck.end());g.hand[0].fill(0);g.hand[0][7]=before.otherHand;
 require(g.observe(1)==before&&encode(g.observe(1))==features,"Hidden information leak");
 Trainer t;for(int i=0;i<19;i++)t.episode();auto initial=t.current.w;auto s=pack(t);Trainer restored=unpack(s);require(pack(restored)==s,"Serialization round trip");
 for(int i=0;i<40;i++){t.episode();restored.episode();}require(pack(t)==pack(restored),"Exact resume RNG/optimizer/partial batch");require(t.current.w!=initial,"Training updates weights");
 auto beforeEval=pack(t);auto e=t.evaluate(20);require(pack(t)==beforeEval,"Evaluation changes training");require(e.games==20,"Evaluation count");
 auto path=std::filesystem::temp_directory_path()/"aa-checkpoint-test"/"test.bin";save(t,path);require(pack(load(path))==pack(t),"File round trip");save(restored,path);require(std::filesystem::exists(path.string()+".bak"),"Backup missing");
 {std::fstream f(path,std::ios::in|std::ios::out|std::ios::binary);f.seekp(40);char z[32]{};f.write(z,32);}bool rejected=false;try{load(path);}catch(...){rejected=true;}require(rejected,"Corrupted checkpoint accepted");require(pack(load(path.string()+".bak"))==pack(t),"Backup recovery");
 // Numerical gradient check for one policy-plus-value objective (advantage held fixed).
 Obs state=Game(rng,0).observe(0);auto xs=encode(state);auto acts=legal(state);auto f=t.current.forward(xs,acts);int selected=acts[0];float adv=1-f.v;
 t.gradient.fill(0);t.samples=0;t.accumulate({state,selected,0},1);
 auto objective=[&](){auto q=t.current.forward(xs,acts);float ent=0;for(int a:acts)ent-=q.p[a]*std::log(q.p[a]);return adv*std::log(q.p[selected])+.02f*ent-.25f*(1-q.v)*(1-q.v);};
 for(int idx:{B1+3,W2+selected*H+2,B2+selected,WV+4,BV}){float old=t.current.w[idx],eps=.001f;t.current.w[idx]=old+eps;float a=objective();t.current.w[idx]=old-eps;float b=objective();t.current.w[idx]=old;require(std::abs((a-b)/(2*eps)-t.gradient[idx])<.002f,"Gradient mismatch");}
 std::cout<<"PASS: 5,000 full games ("<<actions<<" actions), rule edges, hidden-info isolation, exact resume, checksum/backup, evaluation isolation, numerical gradients.\n";
}
int main(int argc,char**argv){try{
 if(argc<2||std::string(argv[1])=="--test"){tests();return 0;}
 if(std::string(argv[1])=="--train"){
  int n=argc>2?std::stoi(argv[2]):1000;std::filesystem::path p=argc>3?argv[3]:"checkpoint.bin";
  Trainer t;if(std::filesystem::exists(p))t=load(p);auto start=std::chrono::steady_clock::now();
  for(int i=0;i<n;i++){t.episode();if(t.games%1000==0){std::cout<<"games="<<t.games<<std::endl;save(t,p);}}
  auto e=t.evaluate(400);t.record(e);save(t,p);std::cout<<"games="<<t.games<<" updates="<<t.updates<<" vs_random="<<e.randomWins<<"/400 vs_heuristic="<<e.greedyWins<<"/400 vs_champion="<<e.championWins<<"/400 seconds="<<std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()<<std::endl;return 0;
 }return 2;
 }catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
