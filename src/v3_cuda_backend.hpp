#pragma once
#include "v3_model.hpp"
#include <cstring>
#include <string>

#ifdef _WIN32
#include <windows.h>
namespace aa {

class V3CudaBackend {
 using CUdevice=int;using CUresult=int;using CUdeviceptr=unsigned long long;using CUcontext=void*;using CUmodule=void*;using CUfunction=void*;
 using nvrtcProgram=void*;using nvrtcResult=int;
 HMODULE driver_=nullptr,nvrtc_=nullptr;CUcontext context_=nullptr;CUmodule module_=nullptr;CUfunction train_=nullptr;
 std::string device_,error_;bool ready_=false;size_t maxBatchSamples_=2048;

 template<class T>T symbol(HMODULE module,const char* name){auto value=reinterpret_cast<T>(GetProcAddress(module,name));if(!value)throw std::runtime_error(std::string("Missing function: ")+name);return value;}
 template<class T>T driver(const char* name){return symbol<T>(driver_,name);}
 template<class T>T nvrtc(const char* name){return symbol<T>(nvrtc_,name);}
 void check(CUresult result,const char* where){if(result){const char* message=nullptr;auto get=reinterpret_cast<CUresult(*)(CUresult,const char**)>(GetProcAddress(driver_,"cuGetErrorString"));if(get)get(result,&message);throw std::runtime_error(std::string(where)+": "+(message?message:"CUDA error"));}}

 static std::string kernel(){return R"CUDA(
#define F 149
#define E 32
#define H 32
#define K 8
#define B 16
#define A 74
#define PI 48
#define ENC_W 0
#define ENC_B 4768
#define WZ 4800
#define UZ 5824
#define BZ 6848
#define WR 6880
#define UR 7904
#define BR 8928
#define WN 8960
#define UN 9984
#define BN 11008
#define BELIEF_W 11040
#define BELIEF_B 11552
#define POLICY_W 11568
#define POLICY_B 15120
#define VALUE_W 15194
#define VALUE_B 15226

__device__ float logistic(float x){return 1.f/(1.f+expf(-fminf(20.f,fmaxf(-20.f,x))));}
__device__ void softmax8(const float* logits,const unsigned char* mask,float* output){
 float maximum=-3.402823466e38f;int count=0;for(int k=0;k<K;k++)if(mask[k]){maximum=fmaxf(maximum,logits[k]);count++;}
 float sum=0;for(int k=0;k<K;k++){output[k]=0;if(mask[k]){output[k]=expf(logits[k]-maximum);sum+=output[k];}}
 if(count)for(int k=0;k<K;k++)if(mask[k])output[k]/=sum;
}
extern "C" __global__ void train_v3(const float* w,const float* x,const int* lengths,const unsigned char* policyMask,const unsigned char* beliefMask,const int* actions,const float* targets,const float* beliefTargets,const unsigned char* beliefEnabled,float* gradient,float* outputs,int maxSteps,int recurrent,int auxiliary,int conditioned,int sequenceLength,float entropyCoefficient,float valueLossWeight,float beliefLossWeight,int batch){
 int sample=blockIdx.x;if(sample>=batch||threadIdx.x)return;int steps=lengths[sample];
 extern __shared__ float storage[];float* embed=storage;float* gateZ=embed+maxSteps*E;float* gateR=gateZ+maxSteps*H;float* candidate=gateR+maxSteps*H;float* hidden=candidate+maxSteps*H;
 const float* input=x+(long long)sample*maxSteps*F;
 for(int t=0;t<steps;t++){
  const float* xt=input+t*F;float* et=embed+t*E;float* ht=hidden+t*H;
  for(int e=0;e<E;e++){float value=w[ENC_B+e];for(int n=0;n<F;n++)value+=w[ENC_W+e*F+n]*xt[n];et[e]=tanhf(value);}
  if(recurrent){
   float* zt=gateZ+t*H;float* rt=gateR+t*H;float* nt=candidate+t*H;
   for(int h=0;h<H;h++){float z=w[BZ+h],r=w[BR+h];for(int e=0;e<E;e++){z+=w[WZ+h*E+e]*et[e];r+=w[WR+h*E+e]*et[e];}if(t)for(int q=0;q<H;q++){float previous=hidden[(t-1)*H+q];z+=w[UZ+h*H+q]*previous;r+=w[UR+h*H+q]*previous;}zt[h]=logistic(z);rt[h]=logistic(r);}
   for(int h=0;h<H;h++){float value=w[BN+h];for(int e=0;e<E;e++)value+=w[WN+h*E+e]*et[e];if(t)for(int q=0;q<H;q++)value+=w[UN+h*H+q]*(rt[q]*hidden[(t-1)*H+q]);nt[h]=tanhf(value);float previous=t?hidden[(t-1)*H+h]:0;ht[h]=(1-zt[h])*nt[h]+zt[h]*previous;}
  }else for(int h=0;h<H;h++)ht[h]=et[h];
 }
 float* finalHidden=hidden+(steps-1)*H;float beliefLogits[B],hand[K],offer[K];
 for(int b=0;b<B;b++){float value=w[BELIEF_B+b];for(int h=0;h<H;h++)value+=w[BELIEF_W+b*H+h]*finalHidden[h];beliefLogits[b]=value;}
 const unsigned char* beliefMaskSample=beliefMask+sample*2*K;softmax8(beliefLogits,beliefMaskSample,hand);softmax8(beliefLogits+K,beliefMaskSample+K,offer);
 float policyInput[PI];for(int h=0;h<H;h++)policyInput[h]=finalHidden[h];for(int k=0;k<2*K;k++)policyInput[H+k]=conditioned?(k<K?hand[k]:offer[k-K]):0;
 const unsigned char* actionMask=policyMask+sample*A;float policy[A],maximum=-3.402823466e38f;
 for(int a=0;a<A;a++){float value=-3.402823466e38f;if(actionMask[a]){value=w[POLICY_B+a];for(int q=0;q<PI;q++)value+=w[POLICY_W+a*PI+q]*policyInput[q];maximum=fmaxf(maximum,value);}policy[a]=value;}
 float sum=0;for(int a=0;a<A;a++)if(actionMask[a]){policy[a]=expf(policy[a]-maximum);sum+=policy[a];}for(int a=0;a<A;a++)policy[a]=actionMask[a]?policy[a]/sum:0;
 float value=w[VALUE_B];for(int h=0;h<H;h++)value+=w[VALUE_W+h]*finalHidden[h];value=tanhf(value);
 float* output=outputs+sample*(2+2*K);output[0]=policy[actions[sample]];output[1]=value;for(int k=0;k<K;k++){output[2+k]=hand[k];output[2+K+k]=offer[k];}
 float entropy=0;for(int a=0;a<A;a++)if(actionMask[a])entropy-=policy[a]*logf(fmaxf(policy[a],1e-9f));float advantage=targets[sample]-value,dPolicy[A];
 for(int a=0;a<A;a++){dPolicy[a]=0;if(actionMask[a]){dPolicy[a]=advantage*((a==actions[sample]?1.f:0.f)-policy[a])-entropyCoefficient*policy[a]*(logf(fmaxf(policy[a],1e-9f))+entropy);atomicAdd(gradient+POLICY_B+a,dPolicy[a]);for(int q=0;q<PI;q++)atomicAdd(gradient+POLICY_W+a*PI+q,dPolicy[a]*policyInput[q]);}}
 float dh[H];for(int h=0;h<H;h++){dh[h]=0;for(int a=0;a<A;a++)if(actionMask[a])dh[h]+=dPolicy[a]*w[POLICY_W+a*PI+h];}
 float dv=valueLossWeight*(targets[sample]-value)*(1-value*value);atomicAdd(gradient+VALUE_B,dv);for(int h=0;h<H;h++){atomicAdd(gradient+VALUE_W+h,dv*finalHidden[h]);dh[h]+=dv*w[VALUE_W+h];}
 float dBelief[B];for(int b=0;b<B;b++)dBelief[b]=0;const float* beliefTarget=beliefTargets+sample*2*K;const unsigned char* enabled=beliefEnabled+sample*2;
 if(auxiliary){if(enabled[0])for(int k=0;k<K;k++)dBelief[k]+=beliefLossWeight*(beliefTarget[k]-hand[k]);if(enabled[1])for(int k=0;k<K;k++)dBelief[K+k]+=beliefLossWeight*(beliefTarget[K+k]-offer[k]);}
 if(conditioned)for(int group=0;group<2;group++){float probabilityGradient[K],dot=0;float* probability=group?offer:hand;for(int k=0;k<K;k++){float d=0;for(int a=0;a<A;a++)if(actionMask[a])d+=dPolicy[a]*w[POLICY_W+a*PI+H+group*K+k];probabilityGradient[k]=d;dot+=probability[k]*d;}for(int k=0;k<K;k++)dBelief[group*K+k]+=probability[k]*(probabilityGradient[k]-dot);}
 for(int b=0;b<B;b++){atomicAdd(gradient+BELIEF_B+b,dBelief[b]);for(int h=0;h<H;h++){atomicAdd(gradient+BELIEF_W+b*H+h,dBelief[b]*finalHidden[h]);dh[h]+=dBelief[b]*w[BELIEF_W+b*H+h];}}
 int stop=steps-min(sequenceLength,steps);
 for(int t=steps-1;t>=stop;t--){float de[E],dhPrevious[H];for(int i=0;i<E;i++)de[i]=0;for(int i=0;i<H;i++)dhPrevious[i]=0;float* et=embed+t*E;
  if(recurrent){float daZ[H],daR[H],daN[H],dq[H];float* zt=gateZ+t*H;float* rt=gateR+t*H;float* nt=candidate+t*H;for(int i=0;i<H;i++)dq[i]=0;
   for(int h=0;h<H;h++){float previous=t?hidden[(t-1)*H+h]:0;daN[h]=dh[h]*(1-zt[h])*(1-nt[h]*nt[h]);daZ[h]=dh[h]*(previous-nt[h])*zt[h]*(1-zt[h]);dhPrevious[h]+=dh[h]*zt[h];}
   for(int h=0;h<H;h++){atomicAdd(gradient+BN+h,daN[h]);for(int e=0;e<E;e++){atomicAdd(gradient+WN+h*E+e,daN[h]*et[e]);de[e]+=daN[h]*w[WN+h*E+e];}for(int q=0;q<H;q++){float previous=t?hidden[(t-1)*H+q]:0;atomicAdd(gradient+UN+h*H+q,daN[h]*(rt[q]*previous));dq[q]+=daN[h]*w[UN+h*H+q];}}
   for(int q=0;q<H;q++){float previous=t?hidden[(t-1)*H+q]:0;daR[q]=dq[q]*previous*rt[q]*(1-rt[q]);dhPrevious[q]+=dq[q]*rt[q];}
   for(int h=0;h<H;h++)for(int gate=0;gate<2;gate++){float da=gate?daR[h]:daZ[h];int wx=gate?WR:WZ,uh=gate?UR:UZ,bias=gate?BR:BZ;atomicAdd(gradient+bias+h,da);for(int e=0;e<E;e++){atomicAdd(gradient+wx+h*E+e,da*et[e]);de[e]+=da*w[wx+h*E+e];}for(int q=0;q<H;q++){float previous=t?hidden[(t-1)*H+q]:0;atomicAdd(gradient+uh+h*H+q,da*previous);dhPrevious[q]+=da*w[uh+h*H+q];}}
  }else for(int e=0;e<E;e++)de[e]=dh[e];
  const float* xt=input+t*F;for(int e=0;e<E;e++){float da=de[e]*(1-et[e]*et[e]);atomicAdd(gradient+ENC_B+e,da);for(int n=0;n<F;n++)atomicAdd(gradient+ENC_W+e*F+n,da*xt[n]);}for(int h=0;h<H;h++)dh[h]=dhPrevious[h];
 }
}
)CUDA";}

 HMODULE findNvrtc(){
  const wchar_t* names[]={L"nvrtc64_130_0.dll",L"nvrtc64_130.dll",L"nvrtc64_129_0.dll",L"nvrtc64_128_0.dll",L"nvrtc64_127_0.dll",L"nvrtc64_126_0.dll",L"nvrtc64_125_0.dll",L"nvrtc64_124_0.dll",L"nvrtc64_122_0.dll",L"nvrtc64_120_0.dll"};for(auto name:names)if(auto module=LoadLibraryW(name))return module;
  wchar_t root[MAX_PATH];DWORD length=GetEnvironmentVariableW(L"CUDA_PATH",root,MAX_PATH);if(length&&length<MAX_PATH){std::wstring pattern=std::wstring(root)+L"\\bin\\nvrtc64_*.dll";WIN32_FIND_DATAW data;HANDLE find=FindFirstFileW(pattern.c_str(),&data);if(find!=INVALID_HANDLE_VALUE){std::wstring path=std::wstring(root)+L"\\bin\\"+data.cFileName;FindClose(find);if(auto module=LoadLibraryExW(path.c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS))return module;}}
  wchar_t programFiles[MAX_PATH];length=GetEnvironmentVariableW(L"ProgramFiles",programFiles,MAX_PATH);if(length&&length<MAX_PATH){const wchar_t* versions[]={L"v13.0",L"v12.9",L"v12.8",L"v12.7",L"v12.6",L"v12.5",L"v12.4",L"v12.3",L"v12.2",L"v12.1",L"v12.0"};for(auto version:versions){std::wstring base=std::wstring(programFiles)+L"\\NVIDIA GPU Computing Toolkit\\CUDA\\"+version+L"\\bin\\";WIN32_FIND_DATAW data;HANDLE find=FindFirstFileW((base+L"nvrtc64_*.dll").c_str(),&data);if(find!=INVALID_HANDLE_VALUE){std::wstring path=base+data.cFileName;FindClose(find);if(auto module=LoadLibraryExW(path.c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS))return module;}}}
  return nullptr;
 }
 void release(){ready_=false;train_=nullptr;module_=nullptr;if(context_&&driver_){auto destroy=reinterpret_cast<CUresult(*)(CUcontext)>(GetProcAddress(driver_,"cuCtxDestroy_v2"));if(destroy)destroy(context_);}context_=nullptr;if(nvrtc_)FreeLibrary(nvrtc_);nvrtc_=nullptr;if(driver_)FreeLibrary(driver_);driver_=nullptr;}

public:
 ~V3CudaBackend(){release();}
 bool init(){try{
  if(ready_)return true;release();error_.clear();driver_=LoadLibraryW(L"nvcuda.dll");if(!driver_)throw std::runtime_error("NVIDIA driver (nvcuda.dll) not found");nvrtc_=findNvrtc();if(!nvrtc_)throw std::runtime_error("CUDA Toolkit NVRTC not found; check CUDA_PATH");
  auto initialize=driver<CUresult(*)(unsigned)>("cuInit");auto getDevice=driver<CUresult(*)(CUdevice*,int)>("cuDeviceGet");auto getName=driver<CUresult(*)(char*,int,CUdevice)>("cuDeviceGetName");auto attribute=driver<CUresult(*)(int*,int,CUdevice)>("cuDeviceGetAttribute");auto createContext=driver<CUresult(*)(CUcontext*,unsigned,CUdevice)>("cuCtxCreate_v2");check(initialize(0),"cuInit");CUdevice device;check(getDevice(&device,0),"cuDeviceGet");char name[128]{};check(getName(name,127,device),"cuDeviceGetName");device_=name;int major=0,minor=0;check(attribute(&major,75,device),"compute major");check(attribute(&minor,76,device),"compute minor");check(createContext(&context_,0,device),"cuCtxCreate");
  auto create=nvrtc<nvrtcResult(*)(nvrtcProgram*,const char*,const char*,int,const char*const*,const char*const*)>("nvrtcCreateProgram");auto compile=nvrtc<nvrtcResult(*)(nvrtcProgram,int,const char*const*)>("nvrtcCompileProgram");auto logSize=nvrtc<nvrtcResult(*)(nvrtcProgram,size_t*)>("nvrtcGetProgramLogSize");auto getLog=nvrtc<nvrtcResult(*)(nvrtcProgram,char*)>("nvrtcGetProgramLog");auto ptxSize=nvrtc<nvrtcResult(*)(nvrtcProgram,size_t*)>("nvrtcGetPTXSize");auto getPtx=nvrtc<nvrtcResult(*)(nvrtcProgram,char*)>("nvrtcGetPTX");auto destroy=nvrtc<nvrtcResult(*)(nvrtcProgram*)>("nvrtcDestroyProgram");
  std::string source=kernel();nvrtcProgram program=nullptr;if(create(&program,source.c_str(),"agent_avenue_v3.cu",0,nullptr,nullptr))throw std::runtime_error("nvrtcCreateProgram failed");std::string architecture="--gpu-architecture=compute_"+std::to_string(major)+std::to_string(minor);const char* options[]={architecture.c_str(),"--std=c++14"};int compiled=compile(program,2,options);if(compiled){size_t size=0;logSize(program,&size);std::string log(size,' ');if(size)getLog(program,log.data());destroy(&program);throw std::runtime_error("CUDA v3 kernel compile failed: "+log);}size_t size=0;ptxSize(program,&size);std::string ptx(size,' ');getPtx(program,ptx.data());destroy(&program);
  auto load=driver<CUresult(*)(CUmodule*,const void*)>("cuModuleLoadData");auto getFunction=driver<CUresult(*)(CUfunction*,CUmodule,const char*)>("cuModuleGetFunction");check(load(&module_,ptx.data()),"cuModuleLoadData");check(getFunction(&train_,module_,"train_v3"),"train_v3 kernel");ready_=true;return true;
 }catch(const std::exception& exception){error_=exception.what();release();return false;}}
 bool ready()const{return ready_;}const std::string& device()const{return device_;}const std::string& error()const{return error_;}
 void setMaxBatchSamples(size_t value){require(value>=32&&value<=8192,"CUDA batch sample limit must be between 32 and 8192");maxBatchSamples_=value;}

 void accumulate(V3Trainer& trainer,const std::vector<V3Sample>& samples,bool applyUpdate=true){
  require(ready_,"V3 CUDA backend is not initialized");if(samples.empty())return;if(samples.size()>maxBatchSamples_){for(size_t begin=0;begin<samples.size();begin+=maxBatchSamples_){size_t end=std::min(samples.size(),begin+maxBatchSamples_);std::vector<V3Sample> part(samples.begin()+begin,samples.begin()+end);accumulate(trainer,part,false);}if(applyUpdate)trainer.update();return;}auto& layout=v3Layout();require(HISTORY_FEATURES==149&&layout.encoderB==4768&&layout.wz==4800&&layout.beliefW==11040&&layout.policyW==11568&&layout.valueW==15194&&layout.total==15227,"V3 CUDA kernel architecture mismatch");int maxSteps=std::max(1,trainer.config.sequenceLength+trainer.config.burnIn);require(maxSteps<=256,"V3 CUDA sequence window exceeds 256 steps");int batch=int(samples.size());
  std::vector<float> inputs(size_t(batch)*maxSteps*HISTORY_FEATURES),targets(batch),beliefTargets(size_t(batch)*2*K),outputs(size_t(batch)*(2+2*K));std::vector<int> lengths(batch),actions(batch);std::vector<unsigned char> policyMasks(size_t(batch)*A),beliefMasks(size_t(batch)*2*K),beliefEnabled(size_t(batch)*2);
  for(int index=0;index<batch;index++){const auto& sample=samples[index];int count=std::min<int>(maxSteps,sample.history.events.size()),begin=int(sample.history.events.size())-count;lengths[index]=count;for(int step=0;step<count;step++){auto encoded=encodeHistoryEvent(sample.history.events[begin+step]);std::copy(encoded.begin(),encoded.end(),inputs.begin()+(size_t(index)*maxSteps+step)*HISTORY_FEATURES);}for(int action:legal(sample.history.events.back().observation))policyMasks[size_t(index)*A+action]=1;auto feasible=feasibleTypes(sample.history.events.back().observation);for(int k=0;k<K;k++){beliefMasks[size_t(index)*2*K+k]=feasible[k];beliefMasks[size_t(index)*2*K+K+k]=sample.history.events.back().observation.phase&&feasible[k];beliefTargets[size_t(index)*2*K+k]=sample.belief.opponentHand[k];beliefTargets[size_t(index)*2*K+K+k]=sample.belief.hiddenOffer[k];}beliefEnabled[size_t(index)*2]=sample.belief.hasOpponentHand;beliefEnabled[size_t(index)*2+1]=sample.belief.hasHiddenOffer;actions[index]=sample.action;targets[index]=sample.target;}
  auto allocate=driver<CUresult(*)(CUdeviceptr*,size_t)>("cuMemAlloc_v2");auto freeMemory=driver<CUresult(*)(CUdeviceptr)>("cuMemFree_v2");auto upload=driver<CUresult(*)(CUdeviceptr,const void*,size_t)>("cuMemcpyHtoD_v2");auto download=driver<CUresult(*)(void*,CUdeviceptr,size_t)>("cuMemcpyDtoH_v2");auto launch=driver<CUresult(*)(CUfunction,unsigned,unsigned,unsigned,unsigned,unsigned,unsigned,unsigned,void*,void**,void**)>("cuLaunchKernel");auto synchronize=driver<CUresult(*)()>("cuCtxSynchronize");
  struct DeviceMemory{CUdeviceptr pointer=0;};std::array<DeviceMemory,11> memory{};auto make=[&](int index,size_t bytes){check(allocate(&memory[index].pointer,bytes),"cuMemAlloc");};
  try{
   make(0,sizeof(float)*trainer.current.weights.size());make(1,sizeof(float)*inputs.size());make(2,sizeof(int)*lengths.size());make(3,policyMasks.size());make(4,beliefMasks.size());make(5,sizeof(int)*actions.size());make(6,sizeof(float)*targets.size());make(7,sizeof(float)*beliefTargets.size());make(8,beliefEnabled.size());make(9,sizeof(float)*trainer.gradient.size());make(10,sizeof(float)*outputs.size());
   check(upload(memory[0].pointer,trainer.current.weights.data(),sizeof(float)*trainer.current.weights.size()),"v3 weights upload");check(upload(memory[1].pointer,inputs.data(),sizeof(float)*inputs.size()),"v3 history upload");check(upload(memory[2].pointer,lengths.data(),sizeof(int)*lengths.size()),"v3 lengths upload");check(upload(memory[3].pointer,policyMasks.data(),policyMasks.size()),"v3 policy masks upload");check(upload(memory[4].pointer,beliefMasks.data(),beliefMasks.size()),"v3 belief masks upload");check(upload(memory[5].pointer,actions.data(),sizeof(int)*actions.size()),"v3 actions upload");check(upload(memory[6].pointer,targets.data(),sizeof(float)*targets.size()),"v3 targets upload");check(upload(memory[7].pointer,beliefTargets.data(),sizeof(float)*beliefTargets.size()),"v3 belief targets upload");check(upload(memory[8].pointer,beliefEnabled.data(),beliefEnabled.size()),"v3 belief flags upload");check(upload(memory[9].pointer,trainer.gradient.data(),sizeof(float)*trainer.gradient.size()),"v3 gradient upload");
   int recurrent=trainer.config.recurrent,auxiliary=trainer.config.beliefAuxiliary,conditioned=trainer.config.beliefConditioned,sequenceLength=trainer.config.sequenceLength;float entropy=trainer.config.entropyCoefficient,valueWeight=trainer.config.valueLossWeight,beliefWeight=trainer.config.beliefLossWeight;void* arguments[]={&memory[0].pointer,&memory[1].pointer,&memory[2].pointer,&memory[3].pointer,&memory[4].pointer,&memory[5].pointer,&memory[6].pointer,&memory[7].pointer,&memory[8].pointer,&memory[9].pointer,&memory[10].pointer,&maxSteps,&recurrent,&auxiliary,&conditioned,&sequenceLength,&entropy,&valueWeight,&beliefWeight,&batch};unsigned sharedBytes=unsigned(size_t(5)*maxSteps*V3_HIDDEN*sizeof(float));check(launch(train_,batch,1,1,1,1,1,sharedBytes,nullptr,arguments,nullptr),"v3 train launch");check(synchronize(),"v3 CUDA synchronize");check(download(trainer.gradient.data(),memory[9].pointer,sizeof(float)*trainer.gradient.size()),"v3 gradient download");check(download(outputs.data(),memory[10].pointer,sizeof(float)*outputs.size()),"v3 metrics download");
  }catch(...){for(auto& item:memory)if(item.pointer)freeMemory(item.pointer);throw;}for(auto& item:memory)if(item.pointer)freeMemory(item.pointer);
  for(int index=0;index<batch;index++){V3Inference output;output.policy[samples[index].action]=outputs[size_t(index)*(2+2*K)];output.value=outputs[size_t(index)*(2+2*K)+1];for(int k=0;k<K;k++){output.opponentHand[k]=outputs[size_t(index)*(2+2*K)+2+k];output.hiddenOffer[k]=outputs[size_t(index)*(2+2*K)+2+K+k];}trainer.recordMetrics(samples[index],output);trainer.pendingSamples++;}if(applyUpdate)trainer.update();
 }
};

} // namespace aa
#else
namespace aa {
class V3CudaBackend {
 std::string error_="V3 CUDA backend is available only in the Windows build";
public:
 bool init(){return false;}bool ready()const{return false;}const std::string& device()const{return error_;}const std::string& error()const{return error_;}
 void setMaxBatchSamples(size_t value){require(value>=32&&value<=8192,"CUDA batch sample limit must be between 32 and 8192");}
 void accumulate(V3Trainer&,const std::vector<V3Sample>&,bool=true){throw std::runtime_error(error_);}
};
} // namespace aa
#endif
