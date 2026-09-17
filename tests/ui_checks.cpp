#define AA_PREVIEW
#include "../src/windows.cpp"
void drawArt(HDC dc,int i,int x,int y,int w,int h){
 if(i<8){const char*files[K]={"double-agent.png","enforcer.png","codebreaker.png","saboteur.png","daredevil.png","sentinel.png","sidekick.png","mole.png"};auto root=fs::current_path();if(!fs::exists(root/"assets"/"cards"))root=fs::absolute(fs::path(__FILE__)).parent_path().parent_path();auto path=(root/"assets"/"cards"/files[i]).generic_string();svg<<"<image x='"<<x<<"' y='"<<y<<"' width='"<<w<<"' height='"<<h<<"' preserveAspectRatio='xMidYMid slice' xlink:href='file://"<<path<<"'/>";return;}
 if(i==8){box(dc,{x,y,x+w,y+h},RGB(31,57,91),white,14);textAt(dc,x+5,y+h/2-15,w-10,30,L"AGENT AI",boldFont,white,DT_CENTER|DT_SINGLELINE);return;}
 box(dc,{x,y,x+w,y+h},RGB(246,248,251),RGB(196,207,220),18);int p[14][2]={{12,50},{12,28},{20,8},{43,5},{66,5},{88,9},{93,30},{93,52},{93,73},{88,93},{66,97},{43,97},{18,93},{12,72}};for(int n=0;n<14;n++){int j=(n+1)%14;line(dc,x+p[n][0]*w/100,y+p[n][1]*h/100,x+p[j][0]*w/100,y+p[j][1]*h/100,RGB(170,184,202),3);box(dc,{x+p[n][0]*w/100-9,y+p[n][1]*h/100-9,x+p[n][0]*w/100+10,y+p[n][1]*h/100+10},white,RGB(154,171,193),18);}
}
void render(const std::string&path){
 svg.str("");svg.clear();hits.clear();clipId=0;svg<<"<svg xmlns='http://www.w3.org/2000/svg' xmlns:xlink='http://www.w3.org/1999/xlink' width='1180' height='940' viewBox='0 0 1180 940'>";
 box(0,{0,0,1180,940},bg,bg,0);textAt(0,30,24,740,46,L"疯狂特务城 · 模型训练与人机对战",titleFont);
 textAt(0,831,37,319,26,L"Public 2.1.0",smallFont,muted,DT_RIGHT|DT_SINGLELINE);
 button(0,30,82,170,35,L"训练与评测",1,true,tab==0);button(0,211,82,170,35,L"人机对战",2,true,tab==1);button(0,392,82,170,35,L"规则与存档",3,true,tab==2);
 if(tab==0)drawTrain(0);else if(tab==1)drawPlay(0);else drawHelp(0);drawInspector(0);svg<<"</svg>";std::ofstream(path)<<svg.str();
}
int main(int argc,char**argv){
 smallFont=16;normalFont=19;boldFont=1020;titleFont=1029;numberFont=1037;tab=1;
 std::string dest=argc>1?argv[1]:"/tmp/aa-preview";fs::create_directories(dest);
 haveGame=false;render(dest+"/gallery.svg");
 newGame(false);render(dest+"/hand.svg");
 auto l=legal(duel.observe(0));applyDuel(l[0]);render(dest+"/own-offer.svg");
 newGame(true);applyDuel(legal(duel.observe(1))[0]);
 render(dest+"/choose.svg");auto hiddenBefore=svg.str();int original=duel.hidden;duel.hidden=(duel.hidden+1)%8;
 render(dest+"/hidden-check.svg");require(svg.str()==hiddenBefore,"Opponent hidden card leaked into rendered UI");duel.hidden=original;
 // Art rendering cannot show an opponent's hidden card: it always receives -1.
 aa::require(!roundReview&&duel.phase&&duel.active==1,"Hidden fixture");
 inspectCard=4;render(dest+"/card-detail.svg");inspectCard=-1;
 newGame(false);duel=Game();duel.active=0;duel.turns=6;duel.progress={1,4};duel.pile[0]={1,1,1,1,1,1,0,0};duel.pile[1]={3,0,2,0,0,1,0,0};
 duel.hand[0]={0,2,0,1,1,0,0,0};duel.hand[1]={0,0,1,1,1,1,0,0};duel.discard[0][0]=duel.discard[1][0]=1;duel.swaps={3,3};
 for(int k=0;k<K;k++){int n=supply[k];for(int p=0;p<2;p++)n-=duel.pile[p][k]+duel.hand[p][k]+duel.discard[p][k];for(int j=0;j<n;j++)duel.deck.push_back(k);}
 applyDuel(3*8+4);applyDuel(73);roundReview=false;applyDuel(5*8+4);applyDuel(72);revealAt=clockMs()-1500;
 require(duel.winner==1&&duel.pile[1][4]==2&&endingLines.size()==1,"UI screenshot result");render(dest+"/screenshot-reproduced.svg");
 // Exercise the production UI transition wrapper for complete games.
 for(int n=0;n<1000;n++){newGame(n%2);int recordedWins=humanWins+humanLosses;
  while(duel.winner<0){roundReview=false;auto actions=legal(duel.observe(duel.actor()));int actor=duel.actor(),phase=duel.phase,face=duel.face,hidden=duel.hidden;int a=actions[playRng()%actions.size()];applyDuel(a);
   if(phase){require(roundReview,"Recruit result not held");require(received[actor]==(a==72?face:hidden),"Displayed recruit owner mismatch");}
   require(duel.conserved(),"UI card conservation");}
  require(humanWins+humanLosses==recordedWins+1&&!endingLines.empty(),"UI winner counted incorrectly or missing reason");
 }
 std::cout<<"PASS: shared UI wrapper 1000 full games, displayed ownership, visible outcome reasons, screenshot reproduction.\n";
}
