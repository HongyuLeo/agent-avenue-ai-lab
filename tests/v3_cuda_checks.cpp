#include "../src/v3_cuda_backend.hpp"
#include "../src/v3_train.hpp"
#include <iostream>

using namespace aa;

int main(){
 try{
  V3CudaBackend cuda;if(!cuda.init()){std::cout<<"SKIP: CUDA unavailable; CPU fallback remains active ("<<cuda.error()<<")\n";return 0;}
  V3Trainer source(91827);auto batch=collectV3Batch(source,source.rng,16,1);require(batch.samples.size()>=40,"CUDA parity batch is too small");batch.samples.resize(40);cuda.setMaxBatchSamples(32);
  V3Trainer cpu=source,gpu=source;for(const auto& sample:batch.samples)cpu.accumulate(sample);cuda.accumulate(gpu,batch.samples,false);
  double differenceSquared=0,referenceSquared=0,maxDifference=0;for(size_t i=0;i<cpu.gradient.size();i++){require(std::isfinite(gpu.gradient[i]),"CUDA produced a non-finite gradient");double difference=std::abs(double(cpu.gradient[i])-gpu.gradient[i]);differenceSquared+=difference*difference;referenceSquared+=double(cpu.gradient[i])*cpu.gradient[i];maxDifference=std::max(maxDifference,difference);}
  double relative=std::sqrt(differenceSquared/std::max(1e-20,referenceSquared));require(relative<2e-3&&maxDifference<2e-2,"CPU/CUDA BPTT gradient parity failed");require(cpu.pendingSamples==gpu.pendingSamples&&cpu.metrics.samples==gpu.metrics.samples,"CUDA metrics/sample accounting mismatch");
  auto before=gpu.current.weights;cpu.update();gpu.update();double maxWeightDifference=0;for(size_t i=0;i<cpu.current.weights.size();i++){require(std::isfinite(gpu.current.weights[i]),"CUDA update produced a non-finite weight");maxWeightDifference=std::max(maxWeightDifference,std::abs(double(cpu.current.weights[i])-gpu.current.weights[i]));}require(maxWeightDifference<2e-4,"CPU/CUDA RMSProp update parity failed");require(gpu.current.weights!=before,"CUDA BPTT did not update v3 weights");
  std::cout<<"PASS: v3 CUDA GRU/BPTT on "<<cuda.device()<<"; samples="<<batch.samples.size()<<" relative_gradient_error="<<relative<<" max_gradient_error="<<maxDifference<<" max_weight_error="<<maxWeightDifference<<"\n";return 0;
 }catch(const std::exception& error){std::cerr<<"FAIL: "<<error.what()<<"\n";return 1;}
}
