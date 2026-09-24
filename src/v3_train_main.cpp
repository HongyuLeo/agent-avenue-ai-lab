#include "v3_train.hpp"
#include "v3_cuda_backend.hpp"
#include <chrono>
#include <iostream>

using namespace aa;

int main(int argc,char** argv){
 try{
  int requested=argc>1?std::stoi(argv[1]):1000,workers=argc>3?std::stoi(argv[3]):int(std::max(1u,std::thread::hardware_concurrency()-1));std::filesystem::path checkpoint=argc>2?argv[2]:"training-v3.bin";std::string requestedDevice=argc>4?argv[4]:"auto";require(requestedDevice=="auto"||requestedDevice=="cpu"||requestedDevice=="cuda","Device must be auto, cpu, or cuda");V3Trainer trainer=std::filesystem::exists(checkpoint)?loadV3(checkpoint):V3Trainer{};if(argc>5){trainer.config.batchGames=std::stoi(argv[5]);require(trainer.config.batchGames>0&&trainer.config.batchGames<=4096,"Batch games must be between 1 and 4096");}
  V3CudaBackend cuda;if(argc>6)cuda.setMaxBatchSamples(size_t(std::stoul(argv[6])));bool useCuda=requestedDevice!="cpu"&&cuda.init();if(requestedDevice=="cuda"&&!useCuda)throw std::runtime_error("CUDA requested but unavailable: "+cuda.error());std::cout<<"device="<<(useCuda?cuda.device():"CPU")<<(requestedDevice=="auto"&&!useCuda?" (CUDA unavailable; automatic fallback)":"")<<"\n";
  auto started=std::chrono::steady_clock::now();int completed=0;while(completed<requested){int games=std::min(trainer.config.batchGames,requested-completed);auto batch=collectV3Batch(trainer,trainer.rng,games,workers);if(useCuda){try{cuda.accumulate(trainer,batch.samples,false);finishV3Batch(trainer,batch.games);}catch(const std::exception& error){if(requestedDevice=="cuda")throw;std::cerr<<"CUDA training failed; falling back to CPU: "<<error.what()<<"\n";useCuda=false;trainV3Batch(trainer,std::move(batch));}}else trainV3Batch(trainer,std::move(batch));completed+=games;if(completed%1024==0||completed==requested){saveV3(trainer,checkpoint);std::cout<<"games="<<trainer.games<<" updates="<<trainer.updates<<"\n";}}
  double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count();std::cout<<"completed="<<completed<<" seconds="<<seconds<<" games_per_s="<<completed/std::max(.001,seconds)<<" training_device="<<(useCuda?cuda.device():"CPU")<<" checkpoint="<<checkpoint.string()<<"\n";return 0;
 }catch(const std::exception& error){std::cerr<<"FAIL: "<<error.what()<<"\n";return 1;}
}
