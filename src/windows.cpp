#define UNICODE
#define _UNICODE
#include "core.hpp"
#ifndef AA_PREVIEW
#include <shellapi.h>
#include <shlobj.h>
#include <gdiplus.h>
#else
#include "preview_adapter.hpp"
#endif
#include "cuda_backend.hpp"
#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <iomanip>
using namespace aa;
namespace fs=std::filesystem;
enum class Language:int{Chinese=0,English=1,Spanish=2};
std::atomic<int> language{int(Language::Chinese)};
struct Localized{
 std::wstring zh,en,es;
 std::wstring get()const{switch(Language(language.load())){case Language::English:return en;case Language::Spanish:return es;default:return zh;}}
};
std::wstring text(const wchar_t* zh,const wchar_t* en,const wchar_t* es){return Localized{zh,en,es}.get();}
const Localized cardNames[K]={
 {L"双面间谍",L"Double Agent",L"Agente doble"},{L"打手",L"Enforcer",L"Matón"},
 {L"解码专家",L"Codebreaker",L"Criptógrafo"},{L"破坏者",L"Saboteur",L"Saboteador"},
 {L"亡命之徒",L"Daredevil",L"Temerario"},{L"哨兵",L"Sentinel",L"Centinela"},
 {L"好搭档",L"Sidekick",L"Compañero"},{L"内鬼",L"Mole",L"Topo"}
};
const wchar_t* englishNames[K]={L"Double Agent",L"Enforcer",L"Codebreaker",L"Saboteur",L"Daredevil",L"Sentinel",L"Sidekick",L"Mole"};
std::wstring cardName(int k){return cardNames[k].get();}
const Localized effects[K]={
 {L"−1 / +6 / −1",L"−1 / +6 / −1",L"−1 / +6 / −1"},{L"+1 / +2 / +3",L"+1 / +2 / +3",L"+1 / +2 / +3"},
 {L"0 / 0 / 获胜",L"0 / 0 / Win",L"0 / 0 / Victoria"},{L"−1 / −1 / −2",L"−1 / −1 / −2",L"−1 / −1 / −2"},
 {L"+2 / +3 / 失败",L"+2 / +3 / Lose",L"+2 / +3 / Derrota"},{L"0 / +2 / +6",L"0 / +2 / +6",L"0 / +2 / +6"},
 {L"+4",L"+4",L"+4"},{L"−3",L"−3",L"−3"}
};
COLORREF accent[K]={RGB(35,116,112),RGB(110,83,162),RGB(29,117,176),RGB(186,95,61),RGB(172,59,69),RGB(48,133,103),RGB(166,125,33),RGB(103,110,129)};
const COLORREF bg=RGB(241,244,248),ink=RGB(25,38,56),muted=RGB(96,112,131),blue=RGB(32,105,214),white=RGB(255,255,255);
HWND windowHandle;HFONT smallFont,normalFont,boldFont,titleFont,numberFont;
std::mutex mx;Trainer trainer;std::thread worker;
std::atomic<bool> running{false},quit{false},evaluateNow{false},busyEval{false};
std::atomic<bool> preferGpu{true},gpuActive{false};
std::atomic<double> gamesPerSecond{0},samplesPerSecond{0};
int trainingWorkers=std::max(1u,std::thread::hardware_concurrency()>2?std::thread::hardware_concurrency()-2:1);
Localized computeText{L"高性能训练尚未启动",L"High-performance training has not started",L"El entrenamiento de alto rendimiento no ha comenzado"};
Localized statusText{L"已就绪。点击开始训练，或直接进入人机对战。",L"Ready. Start training or go directly to Play vs AI.",L"Listo. Inicia el entrenamiento o juega directamente contra la IA."};
fs::path saveDir,savePath,languagePath;std::atomic<uint64_t> savedGames{0};bool closing=false,threadFailed=false;
int tab=0;double rate=0;int chosenUp=-1,chosenDown=-1;bool playBest=false,haveGame=false;
Net playNet;Game duel;std::mt19937 playRng(std::random_device{}());uint64_t playGeneration=0;
int humanWins=0,humanLosses=0;std::vector<Localized>gameLog;
constexpr int CANVAS_H=940;
int inspectCard=-1;bool roundReview=false;uint64_t aiDue=0,revealAt=0;
std::array<int,2> oldProgress{},received{-1,-1};
int lastFace=-1,lastHidden=-1,lastActive=0,lastChooser=0,lastChoice=72;
std::vector<Localized> endingLines;
uint64_t clockMs(){return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();}
struct Hit{RECT r;int id;bool enabled;int card=-1;};std::vector<Hit>hits;
std::wstring num(uint64_t n){return std::to_wstring(n);}
std::wstring pct(int w,int n){if(!n)return text(L"尚未评测",L"Not evaluated",L"Sin evaluar");std::wostringstream s;s<<std::fixed<<std::setprecision(1)<<100.0*w/n<<L"%";return s.str();}
#ifndef AA_PREVIEW
void textAt(HDC dc,int x,int y,int w,int h,const std::wstring&s,HFONT f=normalFont,COLORREF c=ink,UINT flags=DT_LEFT|DT_TOP|DT_WORDBREAK){
 SelectObject(dc,f);SetTextColor(dc,c);SetBkMode(dc,TRANSPARENT);RECT r{x,y,x+w,y+h};DrawTextW(dc,s.c_str(),int(s.size()),&r,flags);
}
void box(HDC dc,RECT r,COLORREF fill,COLORREF border,int radius=14){
 HBRUSH b=CreateSolidBrush(fill);HPEN p=CreatePen(PS_SOLID,1,border);auto ob=SelectObject(dc,b);auto op=SelectObject(dc,p);RoundRect(dc,r.left,r.top,r.right,r.bottom,radius,radius);SelectObject(dc,ob);SelectObject(dc,op);DeleteObject(b);DeleteObject(p);
}
void line(HDC dc,int x,int y,int x2,int y2,COLORREF color,int width=1){HPEN p=CreatePen(PS_SOLID,width,color);auto old=SelectObject(dc,p);MoveToEx(dc,x,y,nullptr);LineTo(dc,x2,y2);SelectObject(dc,old);DeleteObject(p);}
ULONG_PTR gdipToken=0;Gdiplus::Image* cardArt[K]{};
void initArt(){
 Gdiplus::GdiplusStartupInput input;if(Gdiplus::GdiplusStartup(&gdipToken,&input,nullptr)!=Gdiplus::Ok)return;
 wchar_t executable[MAX_PATH]{};DWORD len=GetModuleFileNameW(nullptr,executable,MAX_PATH);if(!len||len>=MAX_PATH)return;
 fs::path root=fs::path(executable).parent_path()/L"assets"/L"cards";
 const wchar_t* files[K]={L"double-agent.png",L"enforcer.png",L"codebreaker.png",L"saboteur.png",L"daredevil.png",L"sentinel.png",L"sidekick.png",L"mole.png"};
 for(int i=0;i<K;i++){auto path=root/files[i];cardArt[i]=Gdiplus::Image::FromFile(path.c_str());if(cardArt[i]&&cardArt[i]->GetLastStatus()!=Gdiplus::Ok){delete cardArt[i];cardArt[i]=nullptr;}}
}
void freeArt(){for(auto&image:cardArt){delete image;image=nullptr;}if(gdipToken)Gdiplus::GdiplusShutdown(gdipToken);}
void drawArt(HDC dc,int i,int x,int y,int w,int h){
 if(i<0||i>=10)return;
 if(i<8){
  if(cardArt[i]){Gdiplus::Graphics g(dc);g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);
   int iw=cardArt[i]->GetWidth(),ih=cardArt[i]->GetHeight(),cropH=std::min(ih,int(iw*double(h)/w)),cropY=(ih-cropH)/2;
   g.DrawImage(cardArt[i],Gdiplus::Rect(x,y,w,h),0,cropY,iw,cropH,Gdiplus::UnitPixel);return;}
  box(dc,{x,y,x+w,y+h},RGB(248,250,253),accent[i],14);box(dc,{x+8,y+8,x+w-8,y+h-8},white,RGB(229,234,241),10);
  int d=std::min(w-24,h/2);box(dc,{x+(w-d)/2,y+20,x+(w+d)/2,y+20+d},accent[i],accent[i],d);
  const wchar_t*mark[K]={L"DA",L"EN",L"CB",L"SA",L"DD",L"SE",L"SK",L"MO"};
  textAt(dc,x+(w-d)/2,y+20,d,d,mark[i],titleFont,white,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
  textAt(dc,x+6,y+h-50,w-12,24,englishNames[i],smallFont,accent[i],DT_CENTER|DT_SINGLELINE);
  textAt(dc,x+6,y+h-27,w-12,20,L"AI LAB",smallFont,muted,DT_CENTER|DT_SINGLELINE);return;
 }
 if(i==8){
  box(dc,{x,y,x+w,y+h},RGB(31,57,91),RGB(20,39,66),14);
  // The diagonal card-back pattern starts outside the card so it covers the
  // whole surface. Clip it to the card bounds or GDI draws those lines across
  // the surrounding offer panel as well.
  int savedDc=SaveDC(dc);IntersectClipRect(dc,x+1,y+1,x+w-1,y+h-1);
  for(int n=-h;n<w;n+=24)line(dc,x+n,y,x+n+h,y+h,RGB(52,85,126),5);
  RestoreDC(dc,savedDc);
  box(dc,{x+12,y+12,x+w-12,y+h-12},RGB(31,57,91),white,10);textAt(dc,x+10,y+h/2-35,w-20,34,L"AGENT",boldFont,white,DT_CENTER|DT_SINGLELINE);textAt(dc,x+10,y+h/2+4,w-20,28,L"AVENUE AI",smallFont,RGB(186,210,239),DT_CENTER|DT_SINGLELINE);return;
 }
 box(dc,{x,y,x+w,y+h},RGB(246,248,251),RGB(196,207,220),18);int pts[14][2]={{12,50},{12,28},{20,8},{43,5},{66,5},{88,9},{93,30},{93,52},{93,73},{88,93},{66,97},{43,97},{18,93},{12,72}};
 for(int n=0;n<14;n++){int j=(n+1)%14;line(dc,x+pts[n][0]*w/100,y+pts[n][1]*h/100,x+pts[j][0]*w/100,y+pts[j][1]*h/100,RGB(170,184,202),3);}
 for(int n=0;n<14;n++){int cx=x+pts[n][0]*w/100,cy=y+pts[n][1]*h/100,r=std::max(8,std::min(w,h)/24);HBRUSH b=CreateSolidBrush(n==0||n==7?RGB(219,232,250):white);HPEN p=CreatePen(PS_SOLID,1,RGB(154,171,193));auto ob=SelectObject(dc,b);auto op=SelectObject(dc,p);Ellipse(dc,cx-r,cy-r,cx+r,cy+r);SelectObject(dc,ob);SelectObject(dc,op);DeleteObject(b);DeleteObject(p);textAt(dc,cx-r,cy-r,r*2,r*2,std::to_wstring(n),smallFont,muted,DT_CENTER|DT_VCENTER|DT_SINGLELINE);}
}
#endif
void button(HDC dc,int x,int y,int w,int h,const std::wstring&s,int id,bool enabled=true,bool primary=false){
 RECT r{x,y,x+w,y+h};box(dc,r,!enabled?RGB(230,234,239):(primary?blue:white),primary?blue:RGB(214,222,232),10);
 textAt(dc,x+8,y,w-16,h,s,boldFont,!enabled?muted:(primary?white:ink),DT_CENTER|DT_VCENTER|DT_SINGLELINE);hits.push_back({r,id,enabled});
}
std::wstring who(int p){return p==0?text(L"你",L"You",L"Tú"):L"AI";}
std::wstring signedMove(int n){return n>0?L"+"+std::to_wstring(n):std::to_wstring(n);}
void addLog(Localized message){gameLog.push_back(std::move(message));}
std::vector<Localized> explainEnd(const Game& g){
 auto out=g.outcome();std::vector<Localized>lines;
 for(int p=0;p<2;p++){
  if(out.caught[p])lines.push_back({
   (p?L"AI":L"你")+std::wstring(L"累计移动领先 ")+num(g.progress[p]-g.progress[1-p])+L" 格，已追上"+(p?L"你":L"AI")+L"。",
   (p?L"The AI":L"You")+std::wstring(L" moved ")+num(g.progress[p]-g.progress[1-p])+L" spaces ahead and caught "+(p?L"you.":L"the AI."),
   (p?L"La IA":L"Tú")+std::wstring(L" lleva ")+num(g.progress[p]-g.progress[1-p])+L" casillas de ventaja y alcanzó "+(p?L"tu ficha.":L"a la IA.")});
  if(out.codebreakers[p])lines.push_back({
   (p?L"AI":L"你")+std::wstring(L"有 ")+num(g.pile[p][2])+L" 张解码专家，满足获胜条件。",
   (p?L"The AI":L"You")+std::wstring(L" collected ")+num(g.pile[p][2])+L" Codebreakers and met the win condition.",
   (p?L"La IA":L"Tú")+std::wstring(L" reunió ")+num(g.pile[p][2])+L" Criptógrafos y cumplió la condición de victoria."});
  if(out.daredevils[p])lines.push_back({
   (p?L"AI":L"你")+std::wstring(L"有 ")+num(g.pile[p][4])+L" 张亡命之徒，满足失败条件。",
   (p?L"The AI":L"You")+std::wstring(L" collected ")+num(g.pile[p][4])+L" Daredevils and met the loss condition.",
   (p?L"La IA":L"Tú")+std::wstring(L" reunió ")+num(g.pile[p][4])+L" Temerarios y cumplió la condición de derrota."});
 }
 if(out.conflict)lines.push_back({
  L"同轮胜负条件冲突；按规则，由本轮出牌方"+(g.active?std::wstring(L"AI"):std::wstring(L"你"))+L"获胜。",
  L"Win and loss conditions conflicted in the same round; the active player "+(g.active?std::wstring(L"AI"):std::wstring(L"(you)"))+L" wins by rule.",
  L"Las condiciones de victoria y derrota coincidieron; por regla gana quien jugó, "+(g.active?std::wstring(L"la IA."):std::wstring(L"tú."))});
 if(out.exhausted)lines.push_back(out.distanceTie?
  Localized{L"牌已耗尽且距离相同；本轮出牌方获胜。",L"The deck is empty and distances are tied; the active player wins.",L"El mazo se agotó con empate de distancia; gana quien jugó la ronda."}:
  Localized{L"牌已耗尽，下一位不足两张手牌；距离更接近的一方获胜。",L"The deck is empty and the next player has fewer than two cards; the closer player wins.",L"El mazo se agotó y el siguiente jugador tiene menos de dos cartas; gana quien esté más cerca."});
 return lines;
}
void scheduleAI(){aiDue=haveGame&&duel.winner<0&&!roundReview&&duel.actor()==1?clockMs()+650:0;}
void applyDuel(int a){
 int actor=duel.actor();bool recruit=duel.phase==1;
 if(recruit){
  oldProgress=duel.progress;lastFace=duel.face;lastHidden=duel.hidden;lastActive=duel.active;lastChooser=actor;lastChoice=a;
  received[actor]=a==72?duel.face:duel.hidden;received[1-actor]=a==72?duel.hidden:duel.face;
 }else if(a>=64)addLog({
  (actor?std::wstring(L"AI"):std::wstring(L"你"))+L"弃掉一张牌并补牌。",
  (actor?std::wstring(L"The AI discards a card and draws a replacement."):std::wstring(L"You discard a card and draw a replacement.")),
  (actor?std::wstring(L"La IA descarta una carta y roba otra."):std::wstring(L"Descartas una carta y robas otra."))});
 else addLog({
  (actor?std::wstring(L"AI"):std::wstring(L"你"))+L"亮出"+cardNames[a/8].zh+L"，另放一张暗牌。",
  (actor?std::wstring(L"The AI reveals "):std::wstring(L"You reveal "))+cardNames[a/8].en+L" and places another card face down.",
  (actor?std::wstring(L"La IA muestra "):std::wstring(L"Muestras "))+cardNames[a/8].es+L" y coloca otra carta boca abajo."});
 duel.step(a);
 if(recruit){
  for(int p=0;p<2;p++){int k=received[p];addLog({
   (p?std::wstring(L"AI"):std::wstring(L"你"))+L"获得第 "+num(duel.pile[p][k])+L" 张"+cardNames[k].zh+L"：移动 "+signedMove(duel.progress[p]-oldProgress[p])+L" 格（累计 "+std::to_wstring(duel.progress[p])+L"）。",
   (p?std::wstring(L"The AI receives its "):std::wstring(L"You receive your "))+num(duel.pile[p][k])+L". "+cardNames[k].en+L": move "+signedMove(duel.progress[p]-oldProgress[p])+L" spaces (total "+std::to_wstring(duel.progress[p])+L").",
   (p?std::wstring(L"La IA recibe su "):std::wstring(L"Recibes tu "))+num(duel.pile[p][k])+L".ª carta "+cardNames[k].es+L": mueve "+signedMove(duel.progress[p]-oldProgress[p])+L" casillas (total "+std::to_wstring(duel.progress[p])+L")."});}
  roundReview=true;revealAt=clockMs();aiDue=0;
 }
 if(duel.winner>=0){
  if(duel.winner==0)humanWins++;else humanLosses++;
  endingLines=explainEnd(duel);for(auto&v:endingLines)addLog(v);addLog({
   L"结算："+(duel.winner?std::wstring(L"AI"):std::wstring(L"你"))+L"获胜。",
   L"Result: "+(duel.winner?std::wstring(L"the AI wins."):std::wstring(L"you win.")),
   L"Resultado: "+(duel.winner?std::wstring(L"gana la IA."):std::wstring(L"ganas tú."))});
 }
 scheduleAI();
}
void advanceAI(){scheduleAI();}
void newGame(bool aiFirst){
 {std::lock_guard<std::mutex>lock(mx);playNet=playBest?trainer.champion:trainer.current;playGeneration=playBest?trainer.championAt:trainer.games;}
 duel.reset(playRng,aiFirst?1:0);haveGame=true;chosenUp=chosenDown=-1;gameLog.clear();endingLines.clear();
 roundReview=false;inspectCard=-1;received={-1,-1};lastFace=lastHidden=-1;revealAt=0;
 addLog({
  L"新局开始："+(aiFirst?std::wstring(L"AI"):std::wstring(L"你"))+L"先手。模型训练局数 "+num(playGeneration)+L"。",
  L"New game: "+(aiFirst?std::wstring(L"the AI moves first. "):std::wstring(L"you move first. "))+L"Model training games: "+num(playGeneration)+L".",
  L"Nueva partida: "+(aiFirst?std::wstring(L"la IA juega primero. "):std::wstring(L"juegas primero. "))+L"Partidas de entrenamiento del modelo: "+num(playGeneration)+L"."});scheduleAI();
}
std::wstring stageText(int k,int stage){if(k==2&&stage==2)return text(L"获胜",L"Win",L"Victoria");if(k==4&&stage==2)return text(L"失败",L"Lose",L"Derrota");return signedMove(moves[k][stage]);}
void card(HDC dc,int x,int y,int w,int h,int k,const std::wstring&label,int id,bool enabled,bool selected=false,int player=-1,bool recruited=false){
 RECT r{x,y,x+w,y+h};box(dc,r,selected?RGB(224,236,255):white,selected?blue:(k>=0?accent[k]:RGB(177,187,201)),12);
 textAt(dc,x+8,y+8,w-16,23,label,smallFont,selected?blue:muted,DT_CENTER|DT_SINGLELINE);
 int ah=h-100,aw=ah*197/274;drawArt(dc,k>=0?k:8,x+(w-aw)/2,y+36,aw,ah);
 if(k>=0){
  textAt(dc,x+6,y+h-60,w-12,25,cardName(k),boldFont,accent[k],DT_CENTER|DT_SINGLELINE);
  int highlight=player>=0?std::min(2,duel.pile[player][k]-(recruited?1:0)):-1;
  int stages=supply[k]==1?1:3;
  for(int j=0;j<stages;j++){int sw=(w-16)/stages,sx=x+8+j*sw;box(dc,{sx,y+h-31,sx+sw-3,y+h-6},j==highlight?accent[k]:RGB(238,241,246),j==highlight?accent[k]:RGB(238,241,246),6);textAt(dc,sx,y+h-30,sw-3,23,stageText(k,j),smallFont,j==highlight?white:muted,DT_CENTER|DT_VCENTER|DT_SINGLELINE);}
 }else textAt(dc,x+8,y+h-52,w-16,42,text(L"暗牌 · 点击选择后揭晓",L"Face-down · revealed after selection",L"Boca abajo · se revela al elegir"),smallFont,muted,DT_CENTER|DT_WORDBREAK);
 if(id>=0)hits.push_back({r,id,enabled,k});
}
void drawTrain(HDC dc){
 uint64_t games,updates,best;Eval e;std::vector<std::array<float,3>>hist;std::wstring status,compute;
 {std::lock_guard<std::mutex>lock(mx);games=trainer.games;updates=trainer.updates;best=trainer.championAt;e=trainer.last;hist=trainer.history;status=statusText.get();compute=computeText.get();}
 box(dc,{30,130,1150,258},white,white);
 const int xx[]={52,330,608,886};const std::wstring labels[]={
  text(L"累计自博弈局数",L"Self-play games",L"Partidas de autojuego"),text(L"参数更新次数",L"Parameter updates",L"Actualizaciones"),
  text(L"最近已保存到",L"Last saved at",L"Último guardado"),text(L"保留模型版本",L"Champion model",L"Modelo campeón")};
 uint64_t vals[]={games,updates,savedGames.load(),best};for(int i=0;i<4;i++){textAt(dc,xx[i],151,248,23,labels[i],smallFont,muted);textAt(dc,xx[i],187,248,48,num(vals[i]),numberFont);}
 button(dc,30,279,200,48,running?text(L"训练进行中",L"Training",L"Entrenando"):text(L"开始 / 继续训练",L"Start / resume training",L"Iniciar / continuar"),10,!running&&!closing,true);
 button(dc,245,279,175,48,text(L"暂停并保存",L"Pause and save",L"Pausar y guardar"),11,running&&!closing);
 button(dc,435,279,175,48,busyEval?text(L"正在评测…",L"Evaluating…",L"Evaluando…"):text(L"评测当前模型",L"Evaluate model",L"Evaluar modelo"),12,!busyEval&&!closing);
 button(dc,625,279,185,48,text(L"打开存档文件夹",L"Open save folder",L"Abrir carpeta"),13);
 button(dc,825,279,325,48,preferGpu?text(L"计算：自动优先 CUDA",L"Compute: prefer CUDA",L"Cálculo: priorizar CUDA"):text(L"计算：仅多核 CPU",L"Compute: multicore CPU",L"Cálculo: CPU multinúcleo"),14,!running);
 textAt(dc,32,344,1110,32,status,normalFont,muted);
 std::wstring speed=gamesPerSecond>0?text(L"速度：",L"Speed: ",L"Velocidad: ")+num(uint64_t(gamesPerSecond.load()))+text(L" 局/秒，",L" games/s, ",L" partidas/s, ")+num(uint64_t(samplesPerSecond.load()))+text(L" 样本/秒　",L" samples/s  ",L" muestras/s  "):L"";
 textAt(dc,32,374,1110,25,speed+compute,smallFont,gpuActive?RGB(25,133,91):muted);
 box(dc,{30,400,748,720},white,white);
 textAt(dc,52,420,620,27,text(L"训练评测记录",L"Evaluation history",L"Historial de evaluación"),boldFont);textAt(dc,52,452,650,22,text(L"蓝：对随机策略　绿：对固定策略（每项 200 局）",L"Blue: random · Green: heuristic (200 games each)",L"Azul: aleatoria · Verde: heurística (200 partidas)"),smallFont,muted);
 int gx=80,gy=665,gw=620,gh=155;
 for(int i=0;i<3;i++){int y=gy-i*gh/2;line(dc,gx,y,gx+gw,y,RGB(227,233,241));textAt(dc,40,y-10,38,20,num(i*50)+L"%",smallFont,muted);}
 if(hist.empty())textAt(dc,160,555,470,40,text(L"完成首次评测后，这里显示实际胜率。",L"Win rates appear here after the first evaluation.",L"Las tasas de victoria aparecerán tras la primera evaluación."),normalFont,muted);
 else{
  float maxx=std::max(1.f,hist.back()[0]);for(int series=1;series<=2;series++){COLORREF c=series==1?blue:RGB(33,147,119);
   for(size_t i=0;i<hist.size();i++){int x=gx+int(gw*hist[i][0]/maxx),y=gy-int(gh*hist[i][series]);if(i){int px=gx+int(gw*hist[i-1][0]/maxx),py=gy-int(gh*hist[i-1][series]);line(dc,px,py,x,y,c,2);}box(dc,{x-3,y-3,x+4,y+4},c,c,4);}}
 }
 box(dc,{770,400,1150,720},white,white);textAt(dc,792,420,332,26,text(L"最近一次评测",L"Latest evaluation",L"Última evaluación"),boldFont);
 textAt(dc,792,468,320,32,text(L"对随机策略　",L"vs random  ",L"vs aleatoria  ")+pct(e.randomWins,e.games));
 textAt(dc,792,512,320,32,text(L"对固定策略　",L"vs heuristic  ",L"vs heurística  ")+pct(e.greedyWins,e.games));
 textAt(dc,792,556,320,32,text(L"对保留模型　",L"vs champion  ",L"vs campeón  ")+pct(e.championWins,e.games));
 textAt(dc,792,610,330,82,text(L"单次胜率有抽样波动。自博弈不保证持续变强；可在对战页亲自验证。",L"Each win rate has sampling noise. Self-play does not guarantee continuous improvement; verify it on the play tab.",L"Cada tasa tiene variación muestral. El autojuego no garantiza una mejora continua; compruébalo jugando."),smallFont,muted);
 textAt(dc,32,744,1110,42,text(L"多核并行自博弈 · CUDA 批量反向传播 · 不联网、不消耗 token · 自动保存",L"Parallel self-play · CUDA batch backprop · Offline · No tokens · Autosave",L"Autojuego paralelo · CUDA por lotes · Sin conexión · Sin tokens · Autoguardado"),smallFont,muted);
}
void drawBoard(HDC dc){
 box(dc,{30,253,350,612},white,white);drawArt(dc,9,75,265,230,320);
 const int coords[14][2]={{33,229},{33,136},{53,45},{130,34},{208,34},{283,49},{298,142},{298,231},{298,320},{282,407},{207,427},{130,427},{49,410},{33,317}};
 double amount=roundReview?std::clamp((clockMs()-revealAt)/1000.0,0.0,1.0):1.0;
 auto point=[&](int p){double progress=amount<1?oldProgress[p]+(duel.progress[p]-oldProgress[p])*amount:duel.progress[p];double pos=std::fmod((p?7:0)+progress,14.0);if(pos<0)pos+=14;int i=int(pos),j=(i+1)%14;double t=pos-i;return std::array<double,2>{75+(coords[i][0]*(1-t)+coords[j][0]*t)*230/331,265+(coords[i][1]*(1-t)+coords[j][1]*t)*320/460};};
 auto p0=point(0),p1=point(1);bool overlap=std::hypot(p0[0]-p1[0],p0[1]-p1[1])<30;
 if(overlap){p0[0]-=13;p1[0]+=13;p0[1]-=10;p1[1]+=10;}
 for(int p=0;p<2;p++){auto pt=p? p1:p0;int x=int(pt[0]),y=int(pt[1]);COLORREF c=p?RGB(218,121,34):blue;box(dc,{x-17,y-17,x+18,y+18},c,white,30);textAt(dc,x-17,y-15,35,30,who(p),smallFont,white,DT_CENTER|DT_VCENTER|DT_SINGLELINE);}
 textAt(dc,36,590,307,21,
  text(L"你累计 ",L"You: ",L"Tú: ")+std::to_wstring(duel.progress[0])+text(L" 格 / AI 累计 ",L" spaces / AI: ",L" casillas / IA: ")+std::to_wstring(duel.progress[1])+text(L" 格",L" spaces",L" casillas"),smallFont,muted,DT_CENTER|DT_SINGLELINE);
}
void drawCollection(HDC dc){
 for(int k=0;k<K;k++){
  int x=370+(k%4)*198,y=253+(k/4)*184;RECT r{x,y,x+186,y+175};box(dc,r,white,RGB(223,229,237));
  textAt(dc,x+8,y+8,170,25,cardName(k),boldFont,accent[k]);drawArt(dc,k,x+9,y+42,66,92);
  for(int p=0;p<2;p++){int yy=y+45+p*44;COLORREF c=p?RGB(176,90,30):blue;
   bool danger=k==4&&duel.pile[p][k]>=2,winning=k==2&&duel.pile[p][k]>=2;
   box(dc,{x+84,yy,x+178,yy+34},danger?RGB(255,229,228):winning?RGB(225,247,235):RGB(240,244,249),RGB(240,244,249),7);
   textAt(dc,x+87,yy+4,87,26,who(p)+L" "+num(duel.pile[p][k])+text(L" 张",L"",L""),boldFont,c,DT_CENTER|DT_SINGLELINE);
  }
  textAt(dc,x+8,y+144,173,23,effects[k].get(),smallFont,muted,DT_CENTER|DT_SINGLELINE);hits.push_back({r,400+k,true,k});
 }
}
void drawPlay(HDC dc){
 button(dc,30,130,190,42,text(L"新开一局：我先手",L"You first",L"Tú empiezas"),20,true,true);button(dc,234,130,190,42,text(L"新开一局：AI 先手",L"AI first",L"Empieza IA"),21);
 button(dc,440,130,245,42,playBest?text(L"下局使用：保留模型",L"Model: champion",L"Modelo: campeón"):text(L"下局使用：最新模型",L"Model: latest",L"Modelo: actual"),22);
 button(dc,700,130,172,42,text(L"导出本局记录",L"Export log",L"Exportar"),27,haveGame);
 textAt(dc,890,142,260,26,text(L"你 ",L"You: ",L"Tú: ")+num(humanWins)+text(L" 胜 / ",L" W / ",L" V / ")+num(humanLosses)+text(L" 负",L" L",L" D"),smallFont,muted,DT_RIGHT|DT_SINGLELINE);
 if(!haveGame){
  textAt(dc,35,213,1100,40,text(L"选择先后手，开始游戏。",L"Choose who goes first and start playing.",L"Elige quién empieza y comienza la partida."),titleFont);
  for(int k=0;k<K;k++){int x=38+(k%4)*283,y=292+(k/4)*293;card(dc,x,y,254,270,k,text(L"点击查看卡牌详情",L"Click for card details",L"Pulsa para ver detalles"),400+k,true);}
  return;
 }
 box(dc,{30,186,1150,239},duel.winner>=0?RGB(228,242,234):RGB(230,238,251),white,10);
 std::wstring reason;if(duel.winner>=0){auto outcome=duel.outcome();reason=outcome.conflict?text(L"胜负条件冲突，主动方获胜",L"conflict resolved in favor of active player",L"conflicto resuelto a favor del jugador activo"):
  outcome.caught[duel.winner]?text(L"追上对手",L"caught the opponent",L"alcanzó al rival"):
  outcome.daredevils[1-duel.winner]?text(L"对手集齐 3 张亡命之徒",L"opponent collected 3 Daredevils",L"el rival reunió 3 Temerarios"):
  outcome.codebreakers[duel.winner]?text(L"集齐 3 张解码专家",L"collected 3 Codebreakers",L"reunió 3 Criptógrafos"):text(L"牌库耗尽结算",L"deck exhaustion",L"mazo agotado");}
 std::wstring state=duel.winner>=0?who(duel.winner)+text(L"获胜 · ",L" wins · ",L" gana · ")+reason:
  roundReview?text(L"本轮结算 · 双方获得的牌已揭晓，请确认后继续",L"Round review · both cards revealed; confirm to continue",L"Resumen de ronda · cartas reveladas; confirma para continuar"):
  duel.actor()==1?text(L"AI 正在思考…",L"AI is thinking…",L"La IA está pensando…"):
  duel.phase?text(L"你来招募 · 选择明牌或暗牌",L"Your choice · take the face-up or face-down card",L"Tu elección · toma la carta visible u oculta"):
  text(L"你来出牌 · 先点明牌，再点暗牌",L"Your turn · choose a face-up card, then a face-down card",L"Tu turno · elige primero la carta visible y luego la oculta");
 textAt(dc,43,197,807,32,state,boldFont);
 textAt(dc,849,202,287,25,text(L"牌库 ",L"Deck: ",L"Mazo: ")+num(duel.deck.size())+text(L" 张 · 第 ",L" · Round ",L" · Ronda ")+num(duel.turns+(duel.winner<0&&!roundReview)),smallFont,muted,DT_RIGHT|DT_SINGLELINE);
 drawBoard(dc);drawCollection(dc);
 bool canAct=duel.winner<0&&!roundReview&&duel.actor()==0;
 if(roundReview){
  int faceOwner=lastChoice==72?lastChooser:1-lastChooser,hiddenOwner=1-faceOwner;
  card(dc,30,652,182,240,lastFace,who(faceOwner)+text(L"获得第 ",L" gets #",L" recibe n.º ")+num(duel.pile[faceOwner][lastFace]),400+lastFace,true,false,faceOwner,true);
  card(dc,228,652,182,240,lastHidden,who(hiddenOwner)+text(L"获得第 ",L" gets #",L" recibe n.º ")+num(duel.pile[hiddenOwner][lastHidden]),400+lastHidden,true,false,hiddenOwner,true);
  textAt(dc,30,620,380,28,text(L"本轮：明牌 / 揭晓的暗牌",L"This round: face-up / revealed face-down",L"Esta ronda: visible / oculta revelada"),boldFont);
  if(duel.winner>=0){
   textAt(dc,439,627,700,30,text(L"结算依据（双方移动完后统一判定）",L"Result (checked after both players move)",L"Resultado (se comprueba tras mover ambos)"),boldFont);
   int yy=669;for(auto&v:endingLines){textAt(dc,439,yy,697,52,v.get(),normalFont);yy+=53;}
   textAt(dc,439,853,690,45,text(L"亡命之徒分别计数：你 ",L"Daredevils are counted separately: you ",L"Los Temerarios se cuentan por separado: tú ")+num(duel.pile[0][4])+text(L" 张，AI ",L", AI ",L", IA ")+num(duel.pile[1][4])+text(L" 张；双方合计不触发失败。",L"; their combined total does not trigger a loss.",L"; el total combinado no provoca una derrota."),smallFont,muted);
  }else{
   for(int p=0;p<2;p++){int k=received[p];textAt(dc,441,666+p*58,680,49,who(p)+text(L"招募第 ",L" recruits #",L" recluta n.º ")+num(duel.pile[p][k])+L" "+cardName(k)+text(L"，移动 ",L" and moves ",L" y mueve ")+signedMove(duel.progress[p]-oldProgress[p])+text(L" 格。",L" spaces.",L" casillas."),boldFont,p?RGB(176,90,30):blue);}
   button(dc,440,812,330,54,text(L"看清了，继续下一轮",L"Continue to next round",L"Continuar a la siguiente ronda"),26,true,true);
  }
 }else if(duel.phase){
  bool own=duel.active==0;
  textAt(dc,30,620,380,28,own?text(L"你提供的两张牌 · 等待 AI 选择",L"Your two cards · waiting for AI",L"Tus dos cartas · esperando a la IA"):text(L"AI 提供的两张牌 · 选择一张",L"AI offered two cards · choose one",L"La IA ofrece dos cartas · elige una"),boldFont);
  card(dc,30,652,182,240,duel.face,text(L"明牌",L"Face-up",L"Visible"),200,canAct,false,0);
  card(dc,228,652,182,240,own?duel.hidden:-1,own?text(L"你的暗牌（AI 看不到）",L"Your face-down card (hidden from AI)",L"Tu carta oculta (la IA no la ve)"):text(L"暗牌",L"Face-down",L"Oculta"),201,canAct,false,0);
  textAt(dc,444,625,694,28,text(L"你的手牌 · 仅你可见",L"Your hand · visible only to you",L"Tu mano · solo tú puedes verla"),boldFont);
  int j=0;for(int k=0;k<K;k++)for(int n=0;n<duel.hand[0][k];n++,j++)card(dc,440+j*178,675,163,208,k,text(L"手牌",L"Hand",L"Mano"),400+k,true,false,0);
 }else{
  textAt(dc,30,620,744,28,canAct?text(L"选两张不同名称的牌；右键可放大查看",L"Choose two differently named cards; right-click for details",L"Elige dos cartas de distinto nombre; clic derecho para ampliar"):text(L"你的手牌 · 等待 AI 出牌",L"Your hand · waiting for AI",L"Tu mano · esperando a la IA"),boldFont);
  std::vector<int>cards;for(int k=0;k<K;k++)for(int n=0;n<duel.hand[0][k];n++)cards.push_back(k);
  for(size_t j=0;j<cards.size();j++){std::wstring label=int(j)==chosenUp?text(L"① 明牌",L"① Face-up",L"① Visible"):int(j)==chosenDown?text(L"② 暗牌",L"② Face-down",L"② Oculta"):text(L"点击选择",L"Click to select",L"Pulsa para elegir");card(dc,30+int(j)*184,652,171,240,cards[j],label,100+int(j),canAct,int(j)==chosenUp||int(j)==chosenDown,0);}
  bool valid=false;if(chosenUp>=0&&chosenDown>=0&&chosenUp<int(cards.size())&&chosenDown<int(cards.size())){int a=cards[chosenUp]*8+cards[chosenDown];auto l=legal(duel.observe(0));valid=std::find(l.begin(),l.end(),a)!=l.end();}
  button(dc,795,657,355,49,text(L"确认出牌",L"Confirm cards",L"Confirmar cartas"),23,canAct&&valid,true);button(dc,795,722,168,43,text(L"重新选择",L"Clear selection",L"Borrar selección"),24,canAct);button(dc,980,722,170,43,text(L"换掉所选牌",L"Swap selected",L"Cambiar elegida"),25,canAct&&chosenUp>=0&&duel.swaps[0]>0&&!duel.deck.empty());
  textAt(dc,800,788,340,67,text(L"本局剩余换牌：你 ",L"Swaps left: you ",L"Cambios restantes: tú ")+num(duel.swaps[0])+text(L" 次 / AI ",L" / AI ",L" / IA ")+num(duel.swaps[1])+text(L" 次。\n色块标出你招募下一张时的效果。",L".\nColors show the effect of your next copy.",L".\nEl color indica el efecto de tu próxima copia."),smallFont,muted);
 }
 textAt(dc,30,912,1118,23,text(L"点击招募区卡牌查看详情 · 模型快照：第 ",L"Click a recruited card for details · Model snapshot: game ",L"Pulsa una carta reclutada para ver detalles · Modelo: partida ")+num(playGeneration)+text(L" 局 · 棋子重叠时双方仍保持可见",L" · Both pieces remain visible when overlapping",L" · Ambas fichas siguen visibles al coincidir"),smallFont,muted);
}
void drawInspector(HDC dc){
 if(inspectCard<0)return;int k=inspectCard;hits.clear();
 std::wstring heading=cardName(k);if(Language(language.load())!=Language::English)heading+=L" / "+std::wstring(englishNames[k]);
 box(dc,{198,151,982,895},RGB(255,255,252),RGB(142,157,175),20);textAt(dc,228,171,652,42,heading,titleFont,accent[k]);button(dc,875,170,79,40,text(L"关闭",L"Close",L"Cerrar"),300);
 drawArt(dc,k,231,235,269,374);textAt(dc,533,240,413,42,text(L"效果按你自己的同名牌数量计算",L"Effect depends on your own copies",L"El efecto depende de tus copias"),boldFont);
 for(int j=0;j<(supply[k]==1?1:3);j++){std::wstring rule=text(L"第 ",L"Copy ",L"Copia ")+num(j+1)+(j==2?text(L" 张及以后",L"+",L"+"):text(L" 张",L"",L""))+L"：";
  if(k==2&&j==2)rule+=text(L"触发获胜条件",L"trigger win condition",L"activa la victoria");else if(k==4&&j==2)rule+=text(L"触发失败条件",L"trigger loss condition",L"activa la derrota");else rule+=text(L"移动 ",L"move ",L"mueve ")+signedMove(moves[k][j])+text(L" 格",L" spaces",L" casillas");
  textAt(dc,535,309+j*74,407,50,rule,normalFont,j==2&&k==4?RGB(188,40,40):ink);
 }
 if(supply[k]==1)textAt(dc,535,383,407,61,text(L"整副牌只有这一张，没有第 2、3 档效果。",L"Only one copy exists; there are no second- or third-copy effects.",L"Solo existe una copia; no hay efectos para la segunda o tercera."),normalFont,muted);
 if(haveGame)textAt(dc,535,557,407,81,text(L"当前已招募：\n你 ",L"Currently recruited:\nYou ",L"Reclutadas:\nTú ")+num(duel.pile[0][k])+text(L" 张　/　AI ",L" / AI ",L" / IA ")+num(duel.pile[1][k])+text(L" 张",L"",L""),boldFont);
 textAt(dc,233,656,710,164,text(
  L"双方移动结束后，再统一检查胜负。\n如果同轮出现获胜与失败条件冲突，按官方规则由本轮出牌方获胜。\n你和 AI 的牌分别计算，不把双方数量相加。",
  L"Win and loss conditions are checked after both players move.\nIf they conflict in the same round, the active player wins by the official rule.\nYour cards and the AI's cards are counted separately.",
  L"La victoria y la derrota se comprueban después de mover ambos.\nSi coinciden en la misma ronda, gana quien jugó la ronda.\nTus cartas y las de la IA se cuentan por separado."),normalFont,muted);
 textAt(dc,233,839,714,25,text(L"公开版使用本项目原创角色插画；不包含原作美术资产。",L"This public build uses original project artwork and contains no official game art.",L"Esta versión usa ilustraciones originales del proyecto, no arte oficial."),smallFont,muted);
}
void drawHelp(HDC dc){
 box(dc,{30,130,1150,765},white,white);
 textAt(dc,54,152,1050,40,text(L"普通模式 · 使用说明",L"Standard mode · Instructions",L"Modo normal · Instrucciones"),titleFont);
 std::wstring s=text(
 L"开始训练\n默认使用多核 CPU 并行生成对局，并把批量梯度交给 NVIDIA CUDA GPU。界面显示实际局/秒和样本/秒。CUDA 不可用时自动回退多核 CPU，并显示具体原因。可暂停后切换计算模式。GitHub Release 随附预训练存档；若只构建源码且未放置 training.bin，则从随机模型开始。\n\n"
 L"保存与恢复\n程序每 10 秒自动保存；正常退出会完成当前一局并保存后关闭。下次启动自动读取模型、优化器、待更新梯度和随机状态。断电或强制结束只能恢复至最近一次成功保存；另保留一份备份。存档位于当前 Windows 用户的 LocalAppData / AgentAvenueAI。\n\n"
 L"对战\n双方初始手牌 4 张。主动方出两张不同名称的牌，一明一暗；对方选一张，剩下的一张归主动方。出牌后立即补足手牌。若手牌全部同名，可出两张同名牌。每人整局最多弃牌换牌 4 次，牌库空时不能换牌。\n\n"
 L"胜负\n按这次招募后同名牌数量，执行第 1 / 2 / 3 档移动。第 4 张及以后仍按第 3 档。双方移动完才判胜负：追上对手，或收集 3 张解码专家即获胜；3 张亡命之徒则失败。冲突判定由主动方获胜。牌库耗尽且下一位无法出两张时，比较谁更接近追上对手，平手归主动方。\n\n"
 L"评测与范围\n每训练约 20,000 局自动评测，也可手动评测。各项 200 局，平衡先手与座位，结果有抽样误差。保留模型是通过对战门槛后保存的版本，并非已证明最优。此版本只含双人普通模式，不含黑市卡或扩展。",
 L"Training\nMulticore CPU workers generate games in parallel, while an NVIDIA CUDA GPU processes batch gradients. Live games/s and samples/s are shown. If CUDA is unavailable, the app falls back to multicore CPU and reports why. Pause before switching compute mode. The GitHub Release includes a pretrained checkpoint; source-only builds without training.bin start from random weights.\n\n"
 L"Save and resume\nThe app autosaves every 10 seconds. A normal exit completes the current game and saves before closing. The next launch restores the model, optimizer, pending gradients and random state. Power loss or forced termination can only restore the latest successful save; one backup is kept. Saves are stored in LocalAppData / AgentAvenueAI.\n\n"
 L"Play\nEach player starts with 4 cards. The active player offers two differently named cards, one face up and one face down. The opponent chooses one; the other returns to the active player. The hand is refilled immediately. Two matching cards may be offered only when every card in hand has that name. Each player may swap a card up to 4 times per game.\n\n"
 L"Winning\nMovement uses the first, second or third-copy effect; later copies keep the third effect. Check results only after both players move. Catch the opponent or collect 3 Codebreakers to win; collecting 3 Daredevils loses. A same-round conflict is won by the active player. If the deck is exhausted, the closer player wins; a distance tie goes to the active player.\n\n"
 L"Evaluation and scope\nEvaluation runs about every 20,000 training games or on demand. Each matchup uses 200 balanced games and has sampling error. The champion is a checkpoint that passed a head-to-head threshold, not a proven optimum. This build covers only the standard two-player mode, without Black Market cards or expansions.",
 L"Entrenamiento\nVarios núcleos de CPU generan partidas en paralelo y una GPU NVIDIA CUDA procesa los gradientes por lotes. Se muestran partidas/s y muestras/s. Si CUDA no está disponible, se usa la CPU y se explica el motivo. Pausa antes de cambiar el modo de cálculo. La versión de GitHub incluye un modelo preentrenado; una compilación sin training.bin empieza con pesos aleatorios.\n\n"
 L"Guardado y reanudación\nLa aplicación guarda cada 10 segundos. Al cerrar normalmente, termina la partida actual y guarda el estado. El siguiente inicio restaura el modelo, optimizador, gradientes pendientes y estado aleatorio. Un corte de energía solo permite recuperar el último guardado correcto; también se conserva una copia de seguridad. Los datos están en LocalAppData / AgentAvenueAI.\n\n"
 L"Partida\nCada jugador empieza con 4 cartas. Quien juega ofrece dos cartas de distinto nombre: una visible y otra oculta. El rival elige una y la restante vuelve a quien las ofreció. La mano se repone de inmediato. Solo se permiten dos cartas iguales si toda la mano tiene el mismo nombre. Cada jugador puede cambiar una carta hasta 4 veces por partida.\n\n"
 L"Victoria\nEl movimiento usa el efecto de la primera, segunda o tercera copia; las siguientes mantienen el tercero. El resultado se comprueba después de mover ambos jugadores. Alcanzar al rival o reunir 3 Criptógrafos da la victoria; reunir 3 Temerarios provoca la derrota. Si hay conflicto en la misma ronda, gana quien jugó. Al agotarse el mazo gana quien esté más cerca; el empate favorece a quien jugó.\n\n"
 L"Evaluación y alcance\nSe evalúa cada 20.000 partidas aproximadamente o de forma manual. Cada rival usa 200 partidas equilibradas y existe variación muestral. El modelo campeón superó un umbral directo, pero no es un óptimo demostrado. Esta versión solo incluye el modo normal para dos jugadores, sin cartas de Mercado Negro ni expansiones.");
 textAt(dc,56,211,1058,508,s,smallFont,ink);
}
#ifndef AA_PREVIEW
std::wstring appTitle(){return text(L"疯狂特务城 · 模型训练与人机对战",L"Agent Avenue AI · Training and Play",L"Agent Avenue IA · Entrenamiento y partida");}
void saveLanguage(){
 try{std::ofstream out(languagePath,std::ios::binary|std::ios::trunc);int value=language.load();out<<(value==1?"en":value==2?"es":"zh");}catch(...){}
}
void chooseLanguage(Language value){language=int(value);saveLanguage();SetWindowTextW(windowHandle,appTitle().c_str());}
void paint(HWND hwnd){
 PAINTSTRUCT ps;HDC target=BeginPaint(hwnd,&ps);RECT client;GetClientRect(hwnd,&client);HDC dc=CreateCompatibleDC(target);HBITMAP bitmap=CreateCompatibleBitmap(target,1180,CANVAS_H);auto old=SelectObject(dc,bitmap);
 HBRUSH brush=CreateSolidBrush(bg);RECT canvas{0,0,1180,CANVAS_H};FillRect(dc,&canvas,brush);DeleteObject(brush);hits.clear();
 textAt(dc,30,24,780,46,appTitle(),titleFont);textAt(dc,831,37,319,26,L"Agent Avenue AI  /  Public 2.2.1",smallFont,muted,DT_RIGHT|DT_SINGLELINE);
 button(dc,30,82,180,35,text(L"训练与评测",L"Training",L"Entrenar"),1,true,tab==0);button(dc,220,82,180,35,text(L"人机对战",L"Play vs AI",L"Jugar vs IA"),2,true,tab==1);button(dc,410,82,180,35,text(L"规则与存档",L"Rules & saves",L"Reglas y datos"),3,true,tab==2);
 textAt(dc,580,89,165,24,text(L"语言 / Language",L"Language",L"Idioma"),smallFont,muted,DT_RIGHT|DT_SINGLELINE);
 button(dc,754,82,83,35,L"中文",4,true,language.load()==0);button(dc,846,82,130,35,L"English",5,true,language.load()==1);button(dc,985,82,165,35,L"Español",6,true,language.load()==2);
 if(tab==0)drawTrain(dc);else if(tab==1)drawPlay(dc);else drawHelp(dc);drawInspector(dc);
 double sc=std::min(client.right/1180.0,client.bottom/double(CANVAS_H));int dw=int(1180*sc),dh=int(CANVAS_H*sc),ox=(client.right-dw)/2,oy=(client.bottom-dh)/2;
 // Compose the scaled scene and margins in a second memory buffer. A single
 // BitBlt reaches the window, so Windows never displays the cleared frame
 // between the background fill and the scaled scene.
 int cw=std::max(1L,client.right),ch=std::max(1L,client.bottom);HDC frameDc=CreateCompatibleDC(target);HBITMAP frameBitmap=CreateCompatibleBitmap(target,cw,ch);auto oldFrame=SelectObject(frameDc,frameBitmap);
 HBRUSH backdrop=CreateSolidBrush(bg);FillRect(frameDc,&client,backdrop);DeleteObject(backdrop);
 SetStretchBltMode(frameDc,HALFTONE);SetBrushOrgEx(frameDc,0,0,nullptr);StretchBlt(frameDc,ox,oy,dw,dh,dc,0,0,1180,CANVAS_H,SRCCOPY);
 BitBlt(target,0,0,cw,ch,frameDc,0,0,SRCCOPY);SelectObject(frameDc,oldFrame);DeleteObject(frameBitmap);DeleteDC(frameDc);
 SelectObject(dc,old);DeleteObject(bitmap);DeleteDC(dc);EndPaint(hwnd,&ps);
}
void performSave(){
 std::lock_guard<std::mutex>lock(mx);save(trainer,savePath);savedGames=trainer.games;
}
void background(){
 auto lastSave=std::chrono::steady_clock::now();bool wasRunning=false;
 CudaBackend cuda;bool cudaTried=false;uint64_t nextEvaluation=0;
 try{
  for(;;){
   if(quit){performSave();PostMessageW(windowHandle,WM_APP+1,0,0);return;}
   if(running){
    if(!preferGpu)cudaTried=false;
    if(preferGpu&&!cudaTried){
     cudaTried=true;{std::lock_guard<std::mutex>lock(mx);statusText={L"正在检测 CUDA 并编译 GPU 批量训练内核…",L"Detecting CUDA and compiling the GPU batch kernel…",L"Detectando CUDA y compilando el núcleo de GPU…"};}
     bool ok=cuda.init();gpuActive=ok;std::string detail=ok?cuda.device():cuda.error();std::wstring wdetail(detail.begin(),detail.end());{std::lock_guard<std::mutex>lock(mx);computeText=ok?
      Localized{L"CUDA："+wdetail,L"CUDA: "+wdetail,L"CUDA: "+wdetail}:
      Localized{L"CUDA 不可用，已回退多核 CPU："+wdetail,L"CUDA unavailable; using multicore CPU: "+wdetail,L"CUDA no disponible; se usa CPU multinúcleo: "+wdetail};}
    }
    Trainer work;{std::lock_guard<std::mutex>lock(mx);work=trainer;if(!nextEvaluation)nextEvaluation=(trainer.games/20000+1)*20000;}
    const int batchGames=std::max(256,trainingWorkers*16),updateChunks=std::max(1,batchGames/16);
    auto begin=std::chrono::steady_clock::now();auto batch=collectBatch(work,work.rng,batchGames,trainingWorkers);
    if(preferGpu&&cuda.ready()){
     try{cuda.train(work,batch.samples,updateChunks);gpuActive=true;}
     catch(const std::exception&e){std::string detail=e.what();std::wstring wdetail(detail.begin(),detail.end());{std::lock_guard<std::mutex>lock(mx);computeText={L"CUDA 批量失败，本批已改用多核 CPU："+wdetail,L"CUDA batch failed; this batch used multicore CPU: "+wdetail,L"Falló el lote CUDA; este lote usó CPU multinúcleo: "+wdetail};}cpuChunkedUpdate(work,batch.samples,updateChunks);gpuActive=false;preferGpu=false;}
    }else{cpuChunkedUpdate(work,batch.samples,updateChunks);gpuActive=false;}
    uint64_t before=work.games;work.games+=batch.games;work.batchGames=work.samples=0;work.gradient.fill(0);
    if(before/2000!=work.games/2000){work.pool.push_back(work.current);if(work.pool.size()>8)work.pool.erase(work.pool.begin()+1);}
    double elapsed=std::max(.001,std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count());gamesPerSecond=batch.games/elapsed;samplesPerSecond=batch.samples.size()/elapsed;
    {std::lock_guard<std::mutex>lock(mx);trainer=std::move(work);statusText=gpuActive?
     Localized{L"CUDA GPU 批量训练中；CPU 正在并行生成对局。",L"CUDA GPU is training batches while the CPU generates games in parallel.",L"La GPU CUDA entrena lotes mientras la CPU genera partidas en paralelo."}:
     Localized{L"多核 CPU 训练中；CUDA 模式可在暂停后切换。",L"Training on multicore CPU; pause to switch back to CUDA.",L"Entrenando con CPU multinúcleo; pausa para volver a CUDA."};}
    wasRunning=true;if(trainer.games>=nextEvaluation){evaluateNow=true;nextEvaluation=(trainer.games/20000+1)*20000;}
   }else if(wasRunning){performSave();wasRunning=false;std::lock_guard<std::mutex>lock(mx);statusText={L"已暂停并保存。下次可从这里继续。",L"Paused and saved. You can resume from here.",L"Pausado y guardado. Puedes continuar desde aquí."};}
   if(evaluateNow.exchange(false)){
    busyEval=true;Trainer snapshot;{std::lock_guard<std::mutex>lock(mx);snapshot=trainer;statusText={L"正在进行 600 局独立评测，训练暂时等待。",L"Running 600 independent evaluation games; training is waiting.",L"Ejecutando 600 partidas de evaluación; el entrenamiento está en espera."};}
    Eval e=snapshot.evaluate(200);{std::lock_guard<std::mutex>lock(mx);trainer.record(e);statusText=running?
     Localized{L"评测完成，继续训练。胜率记录已更新。",L"Evaluation complete. Training resumed and win rates were updated.",L"Evaluación completada. El entrenamiento continúa y se actualizaron las tasas."}:
     Localized{L"评测完成。可以到人机对战页挑战当前模型。",L"Evaluation complete. Challenge the current model on the play tab.",L"Evaluación completada. Desafía al modelo actual en la pestaña de juego."};}
    busyEval=false;performSave();lastSave=std::chrono::steady_clock::now();
   }
   if(std::chrono::steady_clock::now()-lastSave>std::chrono::seconds(10)){
    performSave();lastSave=std::chrono::steady_clock::now();
   }
   if(!running)std::this_thread::sleep_for(std::chrono::milliseconds(25));
  }
 }catch(const std::exception&e){running=false;busyEval=false;std::string s=e.what();std::wstring ws(s.begin(),s.end());{std::lock_guard<std::mutex>lock(mx);statusText={L"保存或训练失败："+ws,L"Save or training failed: "+ws,L"Falló el guardado o entrenamiento: "+ws};}PostMessageW(windowHandle,WM_APP+2,0,0);}
}
void exportLog(){
 std::wstring logText=text(L"Agent Avenue AI 对局公开记录（v2.2.1）\r\n",L"Agent Avenue AI public game log (v2.2.1)\r\n",L"Registro público de partida de Agent Avenue IA (v2.2.1)\r\n");for(auto&v:gameLog)logText+=v.get()+L"\r\n";
 logText+=text(L"\r\n最终公开招募数：\r\n",L"\r\nFinal recruited cards:\r\n",L"\r\nCartas reclutadas al final:\r\n");for(int k=0;k<K;k++)logText+=cardName(k)+text(L"：你 ",L": you ",L": tú ")+num(duel.pile[0][k])+L" / AI "+num(duel.pile[1][k])+L"\r\n";
 int n=WideCharToMultiByte(CP_UTF8,0,logText.data(),int(logText.size()),nullptr,0,nullptr,nullptr);std::string bytes(n,0);WideCharToMultiByte(CP_UTF8,0,logText.data(),int(logText.size()),bytes.data(),n,nullptr,nullptr);
 auto path=saveDir/L"last-game.txt";std::ofstream out(path,std::ios::binary);out<<"\xef\xbb\xbf"<<bytes;out.close();require(bool(out),"Cannot export game log");ShellExecuteW(windowHandle,L"open",path.c_str(),nullptr,nullptr,SW_SHOWNORMAL);
}
void click(int id){
 if(id==300){inspectCard=-1;return;}if(id>=400&&id<408){inspectCard=id-400;return;}
 if(inspectCard>=0)return;
 if(id==26){roundReview=false;chosenUp=chosenDown=-1;scheduleAI();return;}
 if(id==27){exportLog();return;}
 if(((id>=23&&id<=25)||id==200||id==201||(id>=100&&id<104))&&(!haveGame||duel.winner>=0||roundReview||duel.actor()!=0))return;
 if(id>=1&&id<=3){tab=id-1;return;}
 if(id>=4&&id<=6){chooseLanguage(Language(id-4));return;}
 if((id==10||id==12)&&threadFailed){quit=false;threadFailed=false;worker=std::thread(background);}
 switch(id){
 case 10:{running=true;std::lock_guard<std::mutex>lock(mx);statusText={L"正在进行本地自博弈训练。你可以切到对战页，或随时暂停。",L"Local self-play training is running. You may switch to play or pause at any time.",L"El entrenamiento local por autojuego está activo. Puedes jugar o pausar en cualquier momento."};break;}
 case 11:running=false;break;
 case 12:evaluateNow=true;break;
 case 13:ShellExecuteW(windowHandle,L"open",saveDir.c_str(),nullptr,nullptr,SW_SHOWNORMAL);break;
 case 14:preferGpu=!preferGpu;gpuActive=false;{std::lock_guard<std::mutex>lock(mx);computeText=preferGpu?
  Localized{L"下次训练将自动检测 CUDA",L"CUDA will be detected when training resumes",L"CUDA se detectará al reanudar el entrenamiento"}:
  Localized{L"已选择多核 CPU；暂停时可切回 CUDA",L"Multicore CPU selected; pause to switch back to CUDA",L"CPU multinúcleo seleccionada; pausa para volver a CUDA"};}break;
 case 20:newGame(false);break;
 case 21:newGame(true);break;
 case 22:playBest=!playBest;break;
 case 23:{std::vector<int>cards;for(int k=0;k<K;k++)for(int j=0;j<duel.hand[0][k];j++)cards.push_back(k);if(chosenUp>=0&&chosenDown>=0){applyDuel(cards.at(chosenUp)*8+cards.at(chosenDown));chosenUp=chosenDown=-1;advanceAI();}break;}
 case 24:chosenUp=chosenDown=-1;break;
 case 25:{std::vector<int>cards;for(int k=0;k<K;k++)for(int j=0;j<duel.hand[0][k];j++)cards.push_back(k);if(chosenUp>=0){applyDuel(64+cards.at(chosenUp));chosenUp=chosenDown=-1;}break;}
 case 200:case 201:applyDuel(id==200?72:73);advanceAI();break;
 default:if(id>=100&&id<104){int j=id-100;if(chosenUp==j){chosenUp=chosenDown;chosenDown=-1;}else if(chosenDown==j)chosenDown=-1;else if(chosenUp<0)chosenUp=j;else chosenDown=j;}
 }
}
LRESULT CALLBACK proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp){
 switch(msg){
 case WM_PAINT:paint(hwnd);return 0;
 case WM_ERASEBKGND:return 1;
 case WM_TIMER:{
 bool redraw=wp==2&&tab==0;uint64_t now=clockMs();
 if(!closing&&inspectCard<0&&aiDue&&now>=aiDue){aiDue=0;try{if(haveGame&&!roundReview&&duel.winner<0&&duel.actor()==1){applyDuel(playNet.act(duel.observe(1),playRng));redraw=true;}}catch(const std::exception&e){std::string s=e.what();MessageBoxW(hwnd,std::wstring(s.begin(),s.end()).c_str(),text(L"AI 行动失败",L"AI action failed",L"Falló la acción de la IA").c_str(),MB_ICONERROR);}}
 if(tab==1&&roundReview&&now<revealAt+1100)redraw=true;
 if(redraw)InvalidateRect(hwnd,nullptr,FALSE);return 0;}
 case WM_SIZE:InvalidateRect(hwnd,nullptr,FALSE);return 0;
 case WM_GETMINMAXINFO:{auto p=reinterpret_cast<MINMAXINFO*>(lp);p->ptMinTrackSize={960,710};return 0;}
 case WM_KEYDOWN:if(wp==VK_ESCAPE){inspectCard=-1;InvalidateRect(hwnd,nullptr,FALSE);}return 0;
 case WM_RBUTTONUP:case WM_LBUTTONUP:{if(closing)return 0;RECT r;GetClientRect(hwnd,&r);double sc=std::min(r.right/1180.0,r.bottom/double(CANVAS_H));int dw=int(1180*sc),dh=int(CANVAS_H*sc);
  POINT p{int((int(short(LOWORD(lp)))-(r.right-dw)/2)/sc),int((int(short(HIWORD(lp)))-(r.bottom-dh)/2)/sc)};
  try{for(auto h:hits)if(PtInRect(&h.r,p)){if(msg==WM_RBUTTONUP){if(h.card>=0)inspectCard=h.card;}else if(h.enabled)click(h.id);break;}}catch(const std::exception&e){std::string s=e.what();MessageBoxW(hwnd,std::wstring(s.begin(),s.end()).c_str(),text(L"操作未完成",L"Action failed",L"No se pudo completar la acción").c_str(),MB_ICONERROR);}
  InvalidateRect(hwnd,nullptr,FALSE);return 0;}
 case WM_CLOSE:if(!closing){
  if(threadFailed){try{performSave();DestroyWindow(hwnd);}catch(...){if(MessageBoxW(hwnd,text(L"当前进度保存失败。是否退出并保留最近一次成功保存的进度？",L"The current progress could not be saved. Exit and keep the latest successful save?",L"No se pudo guardar el progreso actual. ¿Salir y conservar el último guardado correcto?").c_str(),text(L"无法保存",L"Save failed",L"Error al guardar").c_str(),MB_YESNO|MB_ICONERROR)==IDYES)DestroyWindow(hwnd);}return 0;}
  closing=true;running=false;quit=true;SetWindowTextW(hwnd,text(L"正在保存训练进度，请稍候…",L"Saving training progress…",L"Guardando el progreso…").c_str());}return 0;
 case WM_APP+1:DestroyWindow(hwnd);return 0;
 case WM_APP+2:{std::wstring status;{std::lock_guard<std::mutex>lock(mx);status=statusText.get();}
  if(worker.joinable())worker.join();threadFailed=true;
  if(closing){if(MessageBoxW(hwnd,(status+text(L"\n是否退出并保留最近一次成功保存的进度？",L"\nExit and keep the latest successful save?",L"\n¿Salir y conservar el último guardado correcto?")).c_str(),text(L"无法保存",L"Save failed",L"Error al guardar").c_str(),MB_YESNO|MB_ICONERROR)==IDYES){DestroyWindow(hwnd);return 0;}}
  else MessageBoxW(hwnd,(status+text(L"\n请检查存档目录的空间与权限。已存在的存档保留；解决问题后可点击继续训练重试。",L"\nCheck the save folder's free space and permissions. Existing saves remain intact; retry after fixing the issue.",L"\nComprueba el espacio y los permisos de la carpeta. Los datos existentes siguen intactos; vuelve a intentarlo después.")).c_str(),text(L"训练 / 存档错误",L"Training / save error",L"Error de entrenamiento / guardado").c_str(),MB_ICONERROR);
  closing=false;quit=false;SetWindowTextW(hwnd,appTitle().c_str());return 0;}
 case WM_DESTROY:PostQuitMessage(0);return 0;
 }return DefWindowProcW(hwnd,msg,wp,lp);
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,PWSTR,int show){
 HANDLE singleton=nullptr;
 try{
  SetProcessDPIAware();wchar_t local[MAX_PATH];require(SUCCEEDED(SHGetFolderPathW(nullptr,CSIDL_LOCAL_APPDATA,nullptr,SHGFP_TYPE_CURRENT,local)),"Cannot locate user data folder");saveDir=fs::path(local)/L"AgentAvenueAI";fs::create_directories(saveDir);savePath=saveDir/L"training.bin";languagePath=saveDir/L"language.txt";
  {std::ifstream in(languagePath,std::ios::binary);std::string code;if(in>>code){if(code=="en")language=int(Language::English);else if(code=="es")language=int(Language::Spanish);}}
  singleton=CreateMutexW(nullptr,TRUE,L"Local\\AgentAvenueAI-v1");if(GetLastError()==ERROR_ALREADY_EXISTS){MessageBoxW(nullptr,text(L"软件已经打开，请切换到已有窗口。",L"The app is already open. Switch to the existing window.",L"La aplicación ya está abierta. Cambia a la ventana existente.").c_str(),appTitle().c_str(),MB_OK);return 0;}
  if(fs::exists(savePath)){
   try{trainer=load(savePath);statusText={L"已恢复训练存档。点击继续训练，或进入人机对战。",L"Training checkpoint restored. Resume training or play against the AI.",L"Se restauró el modelo. Continúa entrenando o juega contra la IA."};}
   catch(...){auto backup=savePath;backup+=L".bak";trainer=load(backup);auto damaged=savePath;damaged+=L".damaged-"+std::to_wstring(GetTickCount64());fs::rename(savePath,damaged);statusText={L"主存档损坏，已从上一份备份恢复。损坏文件已保留。",L"The main checkpoint was damaged. The backup was restored and the damaged file was kept.",L"El archivo principal estaba dañado. Se restauró la copia de seguridad y se conservó el archivo dañado."};}
  }else{
   wchar_t executable[MAX_PATH]{};DWORD len=GetModuleFileNameW(nullptr,executable,MAX_PATH);
   if(len&&len<MAX_PATH){auto starter=fs::path(executable).parent_path()/L"training.bin";if(fs::exists(starter)){trainer=load(starter);statusText={L"已载入发布包提供的预训练模型；训练或退出后会保存到用户目录。",L"Loaded the pretrained model from the release; training or exit will save it to your user folder.",L"Se cargó el modelo preentrenado; al entrenar o salir se guardará en tu carpeta de usuario."};}}
  }
  savedGames=trainer.games;
  auto font=[](int size,int weight){return CreateFontW(-size,0,0,0,weight,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Microsoft YaHei UI");};
  smallFont=font(16,FW_NORMAL);normalFont=font(19,FW_NORMAL);boldFont=font(20,FW_SEMIBOLD);titleFont=font(29,FW_BOLD);numberFont=font(37,FW_BOLD);
  WNDCLASSW wc{};wc.lpfnWndProc=proc;wc.hInstance=instance;wc.lpszClassName=L"AgentAvenueAIWindow";wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);wc.hIcon=LoadIconW(nullptr,IDI_APPLICATION);RegisterClassW(&wc);
  initArt();
  RECT frame{0,0,1180,CANVAS_H};AdjustWindowRect(&frame,WS_OVERLAPPEDWINDOW,FALSE);
  windowHandle=CreateWindowW(wc.lpszClassName,appTitle().c_str(),WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,std::min(frame.right-frame.left,LONG(GetSystemMetrics(SM_CXSCREEN)-60)),std::min(frame.bottom-frame.top,LONG(GetSystemMetrics(SM_CYSCREEN)-90)),nullptr,nullptr,instance,nullptr);require(windowHandle!=nullptr,"Cannot create window");
  ShowWindow(windowHandle,show);SetTimer(windowHandle,1,40,nullptr);SetTimer(windowHandle,2,500,nullptr);worker=std::thread(background);
  MSG msg;while(GetMessageW(&msg,nullptr,0,0)>0){TranslateMessage(&msg);DispatchMessageW(&msg);}quit=true;if(worker.joinable())worker.join();
  freeArt();DeleteObject(smallFont);DeleteObject(normalFont);DeleteObject(boldFont);DeleteObject(titleFont);DeleteObject(numberFont);if(singleton)CloseHandle(singleton);return 0;
 }catch(const std::exception&e){std::string s=e.what();std::wstring ws(s.begin(),s.end());MessageBoxW(nullptr,(text(L"启动失败：",L"Startup failed: ",L"Error al iniciar: ")+ws+text(L"\n未覆盖已有存档。",L"\nExisting saves were not overwritten.",L"\nNo se sobrescribieron los datos existentes.")).c_str(),appTitle().c_str(),MB_ICONERROR);if(singleton)CloseHandle(singleton);return 1;}
}

#endif // AA_PREVIEW
