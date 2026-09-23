#pragma once
#include "core.hpp"
#include <array>
#include <cstdint>

namespace aa {

enum class PublicActionKind : uint8_t { Initial, Offer, Swap, ChooseFace, ChooseHidden };

struct HistoryEvent {
 Obs observation;
 PublicActionKind kind=PublicActionKind::Initial;
 int publicCard=-1;
 int privateCard=-1;
 bool operator==(const HistoryEvent& b)const {
  return observation==b.observation&&kind==b.kind&&publicCard==b.publicCard&&privateCard==b.privateCard;
 }
};

struct PlayerHistory {
 int player=0;
 std::vector<HistoryEvent> events;
 bool operator==(const PlayerHistory& b)const{return player==b.player&&events==b.events;}
};

struct InformationState {
 std::array<PlayerHistory,2> views{{PlayerHistory{0,{}},PlayerHistory{1,{}}}};

 explicit InformationState(const Game& game){
  for(int p=0;p<2;p++)views[p].events.push_back({game.observe(p),PublicActionKind::Initial,-1,-1});
 }

 const PlayerHistory& view(int player)const{
  require(player==0||player==1,"Invalid player view");return views[player];
 }

 void recordAfter(const Game& before,int action,const Game& after){
  const int actor=before.actor();
  PublicActionKind kind=PublicActionKind::Initial;int publicCard=-1;
  if(before.phase==0&&action<64){kind=PublicActionKind::Offer;publicCard=action/8;}
  else if(before.phase==0){kind=PublicActionKind::Swap;}
  else if(action==72){kind=PublicActionKind::ChooseFace;publicCard=before.face;}
  else {kind=PublicActionKind::ChooseHidden;publicCard=before.hidden;}
  for(int p=0;p<2;p++){
   int privateCard=-1;
   if(kind==PublicActionKind::Offer&&p==actor)privateCard=action%8;
   else if(kind==PublicActionKind::Swap&&p==actor)privateCard=action-64;
   views[p].events.push_back({after.observe(p),kind,publicCard,privateCard});
  }
 }
};

inline void step(Game& game,InformationState& information,int action){
 Game before=game;game.step(action);information.recordAfter(before,action,game);
}

struct BeliefTarget {
 std::array<float,K> opponentHand{};
 std::array<float,K> hiddenOffer{};
 bool hasOpponentHand=false,hasHiddenOffer=false;
};

// Simulator-only training label. Never pass this object to an inference API.
inline BeliefTarget makeBeliefTarget(const Game& game,int player){
 require(player==0||player==1,"Invalid belief player");BeliefTarget target;
 int count=game.countHand(1-player);target.hasOpponentHand=count>0;
 if(count)for(int k=0;k<K;k++)target.opponentHand[k]=float(game.hand[1-player][k])/count;
 target.hasHiddenOffer=game.phase==1&&game.actor()==player;
 if(target.hasHiddenOffer)target.hiddenOffer[game.hidden]=1.f;
 return target;
}

constexpr int HISTORY_FEATURES=N+5+K+K;
inline std::array<float,HISTORY_FEATURES> encodeHistoryEvent(const HistoryEvent& event){
 std::array<float,HISTORY_FEATURES> result{};auto observation=encode(event.observation);
 std::copy(observation.begin(),observation.end(),result.begin());int j=N;
 result[j+int(event.kind)]=1.f;j+=5;
 if(event.publicCard>=0)result[j+event.publicCard]=1.f;j+=K;
 if(event.privateCard>=0)result[j+event.privateCard]=1.f;
 return result;
}

} // namespace aa
