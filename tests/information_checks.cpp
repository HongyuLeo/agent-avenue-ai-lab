#include "../src/information_state.hpp"
#include <algorithm>
#include <iostream>

using namespace aa;

int main(){
 try{
  std::mt19937 rng(90421);Game original(rng,0);InformationState info(original);
  int offer=-1;for(int a:legal(original.observe(0)))if(a<64){offer=a;break;}
  require(offer>=0,"No test offer");Game before=original;step(original,info,offer);
  require(info.view(0).events.back().privateCard==offer%8,"Offer maker lost own hidden card");
  require(info.view(1).events.back().privateCard==-1,"Face-down offer leaked to responder history");
  require(info.view(1).events.back().publicCard==offer/8,"Face-up offer missing from history");

  Game twin=before;int hidden=offer%8,replacement=(hidden+1)%K;
  while(replacement==offer/8)replacement=(replacement+1)%K;
  require(twin.hand[0][hidden]>0,"Test setup lacks hidden card");
  if(twin.hand[0][replacement]==0){
   auto it=std::find(twin.deck.begin(),twin.deck.end(),replacement);
   require(it!=twin.deck.end(),"No replacement card");*it=hidden;twin.hand[0][hidden]--;twin.hand[0][replacement]++;
  }
  int twinOffer=(offer/8)*8+replacement;auto twinActions=legal(twin.observe(0));
  require(std::find(twinActions.begin(),twinActions.end(),twinOffer)!=twinActions.end(),"Twin offer illegal");
  InformationState twinInfo(twin);step(twin,twinInfo,twinOffer);
  require(info.view(1)==twinInfo.view(1),"Hidden offer changed responder history");
  require(encodeHistoryEvent(info.view(1).events.back())==encodeHistoryEvent(twinInfo.view(1).events.back()),"Hidden offer changed encoded history");
  require(!(makeBeliefTarget(original,1).hiddenOffer==makeBeliefTarget(twin,1).hiddenOffer),"Belief target did not reflect simulator truth");

  auto responderLegal=legal(original.observe(1));auto twinLegal=legal(twin.observe(1));
  require(responderLegal==twinLegal,"Hidden offer changed legal actions");
  require(info.view(0).events.back().privateCard!=-1&&info.view(1).events.back().privateCard==-1,"Player views shared private data");

  Game deckTwin=original;std::reverse(deckTwin.deck.begin(),deckTwin.deck.end());
  require(deckTwin.observe(1)==original.observe(1),"Deck order entered observation");
  require(encode(deckTwin.observe(1))==encode(original.observe(1)),"Deck order entered model input");

  std::cout<<"PASS: player histories sanitize offers/swaps; belief targets remain isolated; hidden hand/offer/deck permutations preserve inference inputs and legal actions.\n";
  return 0;
 }catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}
}
