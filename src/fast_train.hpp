#pragma once
#include "core.hpp"
#include <atomic>
#include <thread>
#include <string>
namespace aa {
struct FastBatch {
 std::vector<Sample> samples;
 uint64_t games=0;
};
inline FastBatch collectBatch(const Trainer& source,std::mt19937& master,int episodes,int workers){
 workers=std::max(1,std::min(workers,episodes));
 std::vector<uint32_t> seeds(workers);for(auto&s:seeds)s=master();
 std::vector<FastBatch> parts(workers);std::vector<std::thread> threads;
 Net current=source.current;auto pool=source.pool;
 for(int w=0;w<workers;w++)threads.emplace_back([&,w]{
  std::mt19937 rng(seeds[w]);auto&out=parts[w];int count=episodes/workers+(w<episodes%workers);
  out.samples.reserve(size_t(count)*18);
  for(int ep=0;ep<count;ep++){
   Game g(rng,int(rng()%2));int learner=int(rng()%2);double mode=uniform(rng);int old=int(rng()%pool.size());size_t begin=out.samples.size();
   while(g.winner<0){int p=g.actor();Obs o=g.observe(p);int a;
    if(p==learner||mode<.50){a=current.act(o,rng);out.samples.push_back({o,a,p});}
    else if(mode<.75)a=pool[old].act(o,rng);
    else if(mode<.9)a=heuristic(o,rng);
    else {auto legalActions=legal(o);a=legalActions[rng()%legalActions.size()];}
    g.step(a);
   }
   float winner=float(g.winner);for(size_t i=begin;i<out.samples.size();i++)out.samples[i].player=(winner==out.samples[i].player)?1:-1;
   out.games++;
  }
 });
 for(auto&t:threads)t.join();
 FastBatch result;size_t total=0;for(auto&p:parts)total+=p.samples.size();result.samples.reserve(total);
 for(auto&p:parts){result.games+=p.games;result.samples.insert(result.samples.end(),std::make_move_iterator(p.samples.begin()),std::make_move_iterator(p.samples.end()));}
 return result;
}
inline void cpuBatchUpdate(Trainer&t,const std::vector<Sample>&samples,int workers){
 if(samples.empty())return;workers=std::max(1,std::min<int>(workers,samples.size()));
 std::vector<std::array<float,P>> gradients(workers);std::vector<int>counts(workers);std::vector<std::thread>threads;
 for(int w=0;w<workers;w++)threads.emplace_back([&,w]{
  Trainer local;local.current=t.current;local.gradient.fill(0);local.samples=0;
  size_t from=samples.size()*w/workers,to=samples.size()*(w+1)/workers;
  for(size_t i=from;i<to;i++)local.accumulate(samples[i],float(samples[i].player));
  gradients[w]=local.gradient;counts[w]=local.samples;
 });
 for(auto&worker:threads)worker.join();
 t.gradient.fill(0);t.samples=0;for(int w=0;w<workers;w++){t.samples+=counts[w];for(int i=0;i<P;i++)t.gradient[i]+=gradients[w][i];}
 t.update();
}
// Preserve the original learner's update cadence while amortizing parallel
// rollout overhead over a much larger collection batch.
inline void cpuChunkedUpdate(Trainer&t,const std::vector<Sample>&samples,int chunks){
 if(samples.empty())return;chunks=std::max(1,std::min<int>(chunks,samples.size()));
 for(int c=0;c<chunks;c++){
  size_t from=samples.size()*c/chunks,to=samples.size()*(c+1)/chunks;
  for(size_t i=from;i<to;i++)t.accumulate(samples[i],float(samples[i].player));
  t.update();
 }
}
struct FastStats {double seconds=0;uint64_t games=0,samples=0;bool gpu=false;std::string device;};
} // namespace aa
