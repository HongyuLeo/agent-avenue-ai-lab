#include "../src/cuda_backend.hpp"
#include <chrono>
#include <iostream>
using namespace aa;
int main(){
 Trainer t;auto original=t;auto start=std::chrono::steady_clock::now();
 auto batch=collectBatch(t,t.rng,512,std::max(2u,std::thread::hardware_concurrency()));
 require(batch.games==512&&batch.samples.size()>2000,"Parallel rollout batch size");
 for(auto&s:batch.samples)require(s.player==1||s.player==-1,"Terminal target missing");
 cpuChunkedUpdate(t,batch.samples,32);t.games+=batch.games;
 require(t.updates==32&&t.games==512&&t.current.w!=original.current.w,"Chunked update did not change model");
 auto after=pack(t);auto restored=unpack(after);require(pack(restored)==after,"Fast checkpoint compatibility");
 // Fixed master RNG must make collection deterministic regardless of thread count.
 Trainer a,b;auto ba=collectBatch(a,a.rng,256,1);auto bb=collectBatch(b,b.rng,256,1);require(ba.samples.size()==bb.samples.size(),"Batch determinism size");
 for(size_t i=0;i<ba.samples.size();i++)require(ba.samples[i].o==bb.samples[i].o&&ba.samples[i].action==bb.samples[i].action&&ba.samples[i].player==bb.samples[i].player,"Batch determinism data");
 double sec=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
 std::cout<<"PASS: parallel rollout/update, terminal targets, checkpoint compatibility; "<<batch.games/sec<<" games/s on this host, "<<batch.samples.size()<<" samples.\n";
}
