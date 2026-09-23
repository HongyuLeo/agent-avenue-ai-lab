#pragma once
#include "v3_model.hpp"
#include <thread>

namespace aa {

struct V3Batch {std::vector<V3Sample> samples;uint64_t games=0;};

inline V3Batch collectV3Batch(const V3Trainer& source,std::mt19937& master,int episodes,int workers){
 workers=std::max(1,std::min(workers,episodes));std::vector<uint32_t> seeds(workers);for(auto& seed:seeds)seed=master();
 std::vector<V3Batch> parts(workers);std::vector<std::thread> threads;
 for(int worker=0;worker<workers;worker++)threads.emplace_back([&,worker]{
  std::mt19937 rng(seeds[worker]);auto& output=parts[worker];int count=episodes/workers+(worker<episodes%workers);
  for(int episode=0;episode<count;episode++){
   Game game(rng,int(rng()%2));InformationState information(game);int learner=int(rng()%2);double mode=uniform(rng);int old=int(rng()%source.pool.size());size_t begin=output.samples.size();
   while(game.winner<0){int player=game.actor();auto playerHistory=information.view(player);int action;
    if(player==learner||mode<.50){action=source.current.act(playerHistory,source.config,rng);output.samples.push_back({std::move(playerHistory),makeBeliefTarget(game,player),action,player,0});}
    else if(mode<.75)action=source.pool[old].act(playerHistory,source.config,rng);else if(mode<.9)action=heuristic(game.observe(player),rng);else{auto actions=legal(game.observe(player));action=actions[rng()%actions.size()];}
    step(game,information,action);
   }
   for(size_t sample=begin;sample<output.samples.size();sample++)output.samples[sample].target=game.winner==output.samples[sample].player?1.f:-1.f;output.games++;
  }
 });
 for(auto& thread:threads)thread.join();V3Batch result;size_t samples=0;for(auto& part:parts)samples+=part.samples.size();result.samples.reserve(samples);
 for(auto& part:parts){result.games+=part.games;result.samples.insert(result.samples.end(),std::make_move_iterator(part.samples.begin()),std::make_move_iterator(part.samples.end()));}return result;
}

inline void finishV3Batch(V3Trainer& trainer,uint64_t batchGames){
 uint64_t before=trainer.games;trainer.games+=batchGames;trainer.pendingGames+=int(batchGames);trainer.update();
 for(uint64_t boundary=(before/2000+1)*2000;boundary<=trainer.games;boundary+=2000){trainer.pool.push_back(trainer.current);if(trainer.pool.size()>8)trainer.pool.erase(trainer.pool.begin()+1);}
}

inline void trainV3Batch(V3Trainer& trainer,V3Batch batch){
 for(auto& sample:batch.samples)trainer.accumulate(sample);finishV3Batch(trainer,batch.games);
}

} // namespace aa
