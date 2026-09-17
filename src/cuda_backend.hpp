#pragma once
#include "fast_train.hpp"
#include <cstring>
#include <string>
#ifdef _WIN32
#include <windows.h>
namespace aa {
class CudaBackend {
 using CUdevice=int;using CUresult=int;using CUdeviceptr=unsigned long long;using CUcontext=void*;using CUmodule=void*;using CUfunction=void*;
 using nvrtcProgram=void*;using nvrtcResult=int;
 HMODULE driver_=nullptr,nvrtc_=nullptr;CUcontext context_=nullptr;CUmodule module_=nullptr;CUfunction forward_=nullptr,errors_=nullptr,gradients_=nullptr,update_=nullptr;
 std::string device_,error_;bool ready_=false;
 template<class T>T sym(HMODULE m,const char*n){auto p=reinterpret_cast<T>(GetProcAddress(m,n));if(!p)throw std::runtime_error(std::string("Missing function: ")+n);return p;}
 template<class T>T driver(const char*n){return sym<T>(driver_,n);}
 template<class T>T nvrtc(const char*n){return sym<T>(nvrtc_,n);}
 static std::string kernel(){return R"CUDA(
#define N 128
#define H 48
#define A 74
#define B1 (N*H)
#define W2 (B1+H)
#define B2 (W2+A*H)
#define WV (B2+A)
#define BV (WV+H)
#define P (BV+1)
extern "C" __global__ void forward(const float*w,const float*x,const unsigned char*mask,float*hidden,float*prob,float*value,int batch){
 int b=blockIdx.x*blockDim.x+threadIdx.x;if(b>=batch)return;const float*xb=x+b*N;float*hb=hidden+b*H;float*pb=prob+b*A;
 for(int h=0;h<H;h++){float v=w[B1+h];for(int n=0;n<N;n++)v+=w[h*N+n]*xb[n];hb[h]=tanhf(v);}
 float mx=-3.402823e38f;for(int a=0;a<A;a++){float v=-3.402823e38f;if(mask[b*A+a]){v=w[B2+a];for(int h=0;h<H;h++)v+=w[W2+a*H+h]*hb[h];mx=fmaxf(mx,v);}pb[a]=v;}
 float sum=0;for(int a=0;a<A;a++)if(mask[b*A+a]){pb[a]=expf(pb[a]-mx);sum+=pb[a];}
 for(int a=0;a<A;a++)pb[a]=mask[b*A+a]?pb[a]/sum:0;
 float v=w[BV];for(int h=0;h<H;h++)v+=w[WV+h]*hb[h];value[b]=tanhf(v);
}
extern "C" __global__ void errors(const float*w,const unsigned char*mask,const int*action,const float*target,const float*hidden,float*prob,float*value,float*dh,int batch){
 int b=blockIdx.x*blockDim.x+threadIdx.x;if(b>=batch)return;float*p=prob+b*A;float entropy=0;
 for(int a=0;a<A;a++)if(mask[b*A+a])entropy-=p[a]*logf(fmaxf(p[a],1e-9f));
 float advantage=target[b]-value[b];for(int a=0;a<A;a++)p[a]=mask[b*A+a]?advantage*((a==action[b])-p[a])-.02f*p[a]*(logf(fmaxf(p[a],1e-9f))+entropy):0;
 float dv=.5f*(target[b]-value[b])*(1-value[b]*value[b]);value[b]=dv;
 for(int h=0;h<H;h++){float d=dv*w[WV+h];for(int a=0;a<A;a++)d+=p[a]*w[W2+a*H+h];float hv=hidden[b*H+h];dh[b*H+h]=d*(1-hv*hv);}
}
extern "C" __global__ void gradients(const float*x,const float*hidden,const float*dout,const float*dv,const float*dh,float*grad,int batch){
 int i=blockIdx.x*blockDim.x+threadIdx.x;if(i>=P)return;float sum=0;
 if(i<B1){int h=i/N,n=i%N;for(int b=0;b<batch;b++)sum+=dh[b*H+h]*x[b*N+n];}
 else if(i<W2){int h=i-B1;for(int b=0;b<batch;b++)sum+=dh[b*H+h];}
 else if(i<B2){int q=i-W2,a=q/H,h=q%H;for(int b=0;b<batch;b++)sum+=dout[b*A+a]*hidden[b*H+h];}
 else if(i<WV){int a=i-B2;for(int b=0;b<batch;b++)sum+=dout[b*A+a];}
 else if(i<BV){int h=i-WV;for(int b=0;b<batch;b++)sum+=dv[b]*hidden[b*H+h];}
 else for(int b=0;b<batch;b++)sum+=dv[b];
 grad[i]=sum/batch;
}
extern "C" __global__ void update(float*w,float*rms,const float*grad){
 if(blockIdx.x||threadIdx.x)return;float norm=0;for(int i=0;i<P;i++)norm+=grad[i]*grad[i];float scale=1.f/fmaxf(1.f,sqrtf(norm));
 for(int i=0;i<P;i++){float g=grad[i]*scale;rms[i]=.99f*rms[i]+.01f*g*g;w[i]+=.00035f*g/(sqrtf(rms[i])+1e-5f);}
}
)CUDA";}
 void check(CUresult r,const char*where){if(r){const char*msg=nullptr;auto f=reinterpret_cast<CUresult(*)(CUresult,const char**)>(GetProcAddress(driver_,"cuGetErrorString"));if(f)f(r,&msg);throw std::runtime_error(std::string(where)+": "+(msg?msg:"CUDA error"));}}
 HMODULE findNvrtc(){
  const wchar_t*names[]={L"nvrtc64_130_0.dll",L"nvrtc64_130.dll",L"nvrtc64_129_0.dll",L"nvrtc64_128_0.dll",L"nvrtc64_127_0.dll",L"nvrtc64_126_0.dll",L"nvrtc64_125_0.dll",L"nvrtc64_124_0.dll",L"nvrtc64_122_0.dll",L"nvrtc64_120_0.dll"};
  for(auto n:names)if(auto m=LoadLibraryW(n))return m;
  wchar_t root[MAX_PATH];DWORD len=GetEnvironmentVariableW(L"CUDA_PATH",root,MAX_PATH);if(len&&len<MAX_PATH){std::wstring pattern=std::wstring(root)+L"\\bin\\nvrtc64_*.dll";WIN32_FIND_DATAW fd;HANDLE h=FindFirstFileW(pattern.c_str(),&fd);if(h!=INVALID_HANDLE_VALUE){std::wstring path=std::wstring(root)+L"\\bin\\"+fd.cFileName;FindClose(h);if(auto m=LoadLibraryExW(path.c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS))return m;}}
  wchar_t programFiles[MAX_PATH];len=GetEnvironmentVariableW(L"ProgramFiles",programFiles,MAX_PATH);if(len&&len<MAX_PATH){
   const wchar_t*versions[]={L"v13.0",L"v12.9",L"v12.8",L"v12.7",L"v12.6",L"v12.5",L"v12.4",L"v12.3",L"v12.2",L"v12.1",L"v12.0"};
   for(auto version:versions){std::wstring base=std::wstring(programFiles)+L"\\NVIDIA GPU Computing Toolkit\\CUDA\\"+version+L"\\bin\\";WIN32_FIND_DATAW fd;HANDLE h=FindFirstFileW((base+L"nvrtc64_*.dll").c_str(),&fd);if(h!=INVALID_HANDLE_VALUE){std::wstring path=base+fd.cFileName;FindClose(h);if(auto m=LoadLibraryExW(path.c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS))return m;}}
  }
  return nullptr;
 }
 void release(){
  ready_=false;module_=nullptr;forward_=errors_=gradients_=update_=nullptr;
  if(context_&&driver_){auto f=reinterpret_cast<CUresult(*)(CUcontext)>(GetProcAddress(driver_,"cuCtxDestroy_v2"));if(f)f(context_);}context_=nullptr;
  if(nvrtc_)FreeLibrary(nvrtc_);nvrtc_=nullptr;if(driver_)FreeLibrary(driver_);driver_=nullptr;
 }
public:
 ~CudaBackend(){release();}
 bool init(){try{
  if(ready_)return true;release();error_.clear();
  driver_=LoadLibraryW(L"nvcuda.dll");if(!driver_)throw std::runtime_error("NVIDIA driver (nvcuda.dll) not found");nvrtc_=findNvrtc();if(!nvrtc_)throw std::runtime_error("CUDA Toolkit NVRTC not found; check CUDA_PATH");
  auto cuInit=driver<CUresult(*)(unsigned)>("cuInit");auto cuDeviceGet=driver<CUresult(*)(CUdevice*,int)>("cuDeviceGet");auto cuDeviceGetName=driver<CUresult(*)(char*,int,CUdevice)>("cuDeviceGetName");auto cuAttr=driver<CUresult(*)(int*,int,CUdevice)>("cuDeviceGetAttribute");auto cuCtxCreate=driver<CUresult(*)(CUcontext*,unsigned,CUdevice)>("cuCtxCreate_v2");
  check(cuInit(0),"cuInit");CUdevice dev;check(cuDeviceGet(&dev,0),"cuDeviceGet");char name[128]{};check(cuDeviceGetName(name,127,dev),"cuDeviceGetName");device_=name;int major=0,minor=0;check(cuAttr(&major,75,dev),"compute major");check(cuAttr(&minor,76,dev),"compute minor");check(cuCtxCreate(&context_,0,dev),"cuCtxCreate");
  auto create=nvrtc<nvrtcResult(*)(nvrtcProgram*,const char*,const char*,int,const char*const*,const char*const*)>("nvrtcCreateProgram");auto compile=nvrtc<nvrtcResult(*)(nvrtcProgram,int,const char*const*)>("nvrtcCompileProgram");auto logSize=nvrtc<nvrtcResult(*)(nvrtcProgram,size_t*)>("nvrtcGetProgramLogSize");auto getLog=nvrtc<nvrtcResult(*)(nvrtcProgram,char*)>("nvrtcGetProgramLog");auto ptxSize=nvrtc<nvrtcResult(*)(nvrtcProgram,size_t*)>("nvrtcGetPTXSize");auto getPtx=nvrtc<nvrtcResult(*)(nvrtcProgram,char*)>("nvrtcGetPTX");auto destroy=nvrtc<nvrtcResult(*)(nvrtcProgram*)>("nvrtcDestroyProgram");
  std::string source=kernel();nvrtcProgram program=nullptr;if(create(&program,source.c_str(),"agent_avenue.cu",0,nullptr,nullptr))throw std::runtime_error("nvrtcCreateProgram failed");std::string arch="--gpu-architecture=compute_"+std::to_string(major)+std::to_string(minor);const char*opts[]={arch.c_str(),"--std=c++14","--use_fast_math"};int cr=compile(program,3,opts);if(cr){size_t n=0;logSize(program,&n);std::string log(n,' ');if(n)getLog(program,log.data());destroy(&program);throw std::runtime_error("CUDA kernel compile failed: "+log);}
  size_t n=0;ptxSize(program,&n);std::string ptx(n,' ');getPtx(program,ptx.data());destroy(&program);
  auto load=driver<CUresult(*)(CUmodule*,const void*)>("cuModuleLoadData");auto getFn=driver<CUresult(*)(CUfunction*,CUmodule,const char*)>("cuModuleGetFunction");check(load(&module_,ptx.data()),"cuModuleLoadData");check(getFn(&forward_,module_,"forward"),"forward kernel");check(getFn(&errors_,module_,"errors"),"errors kernel");check(getFn(&gradients_,module_,"gradients"),"gradients kernel");check(getFn(&update_,module_,"update"),"update kernel");ready_=true;return true;
 }catch(const std::exception&e){error_=e.what();release();return false;}}
 bool ready()const{return ready_;}const std::string&device()const{return device_;}const std::string&error()const{return error_;}
 void train(Trainer&t,const std::vector<Sample>&samples,int chunks=1){
  if(!ready_||samples.empty())return;int total=int(samples.size());chunks=std::max(1,std::min(chunks,total));int capacity=(total+chunks-1)/chunks;
  std::vector<float>x(size_t(total)*N),target(total);std::vector<unsigned char>mask(size_t(total)*A);std::vector<int>action(total);
  for(int b=0;b<total;b++){auto e=encode(samples[b].o);std::copy(e.begin(),e.end(),x.begin()+size_t(b)*N);for(int a:legal(samples[b].o))mask[size_t(b)*A+a]=1;action[b]=samples[b].action;target[b]=float(samples[b].player);}
  auto alloc=driver<CUresult(*)(CUdeviceptr*,size_t)>("cuMemAlloc_v2");auto free=driver<CUresult(*)(CUdeviceptr)>("cuMemFree_v2");auto h2d=driver<CUresult(*)(CUdeviceptr,const void*,size_t)>("cuMemcpyHtoD_v2");auto d2h=driver<CUresult(*)(void*,CUdeviceptr,size_t)>("cuMemcpyDtoH_v2");auto launch=driver<CUresult(*)(CUfunction,unsigned,unsigned,unsigned,unsigned,unsigned,unsigned,unsigned,void*,void**,void**)>("cuLaunchKernel");auto sync=driver<CUresult(*)()>("cuCtxSynchronize");
  struct Mem{CUdeviceptr p=0;};std::vector<Mem>mem(11);auto make=[&](int i,size_t bytes){check(alloc(&mem[i].p,bytes),"cuMemAlloc");};
  try{make(0,sizeof(float)*P);make(1,sizeof(float)*P);make(2,sizeof(float)*x.size());make(3,mask.size());make(4,sizeof(int)*action.size());make(5,sizeof(float)*target.size());make(6,sizeof(float)*size_t(capacity)*H);make(7,sizeof(float)*size_t(capacity)*A);make(8,sizeof(float)*capacity);make(9,sizeof(float)*size_t(capacity)*H);make(10,sizeof(float)*P);
   check(h2d(mem[0].p,t.current.w.data(),sizeof(float)*P),"weights upload");check(h2d(mem[1].p,t.rms.data(),sizeof(float)*P),"optimizer upload");check(h2d(mem[2].p,x.data(),sizeof(float)*x.size()),"observations upload");check(h2d(mem[3].p,mask.data(),mask.size()),"masks upload");check(h2d(mem[4].p,action.data(),sizeof(int)*action.size()),"actions upload");check(h2d(mem[5].p,target.data(),sizeof(float)*target.size()),"targets upload");
   for(int c=0;c<chunks;c++){
    int from=total*c/chunks,batch=total*(c+1)/chunks-from;CUdeviceptr dx=mem[2].p+sizeof(float)*size_t(from)*N,dmask=mem[3].p+size_t(from)*A,daction=mem[4].p+sizeof(int)*size_t(from),dtarget=mem[5].p+sizeof(float)*size_t(from);
    unsigned blocks=(batch+127)/128;void*fargs[]={&mem[0].p,&dx,&dmask,&mem[6].p,&mem[7].p,&mem[8].p,&batch};check(launch(forward_,blocks,1,1,128,1,1,0,nullptr,fargs,nullptr),"forward launch");
    void*eargs[]={&mem[0].p,&dmask,&daction,&dtarget,&mem[6].p,&mem[7].p,&mem[8].p,&mem[9].p,&batch};check(launch(errors_,blocks,1,1,128,1,1,0,nullptr,eargs,nullptr),"errors launch");
    void*gargs[]={&dx,&mem[6].p,&mem[7].p,&mem[8].p,&mem[9].p,&mem[10].p,&batch};check(launch(gradients_,(P+255)/256,1,1,256,1,1,0,nullptr,gargs,nullptr),"gradients launch");
    void*uargs[]={&mem[0].p,&mem[1].p,&mem[10].p};check(launch(update_,1,1,1,1,1,1,0,nullptr,uargs,nullptr),"update launch");
   }
   check(sync(),"CUDA synchronize");
   check(d2h(t.current.w.data(),mem[0].p,sizeof(float)*P),"weights download");check(d2h(t.rms.data(),mem[1].p,sizeof(float)*P),"optimizer download");t.updates++;
   t.updates+=chunks-1;
  }catch(...){for(auto&m:mem)if(m.p)free(m.p);throw;}for(auto&m:mem)if(m.p)free(m.p);
 }
};
} // namespace aa
#else
namespace aa {class CudaBackend {std::string error_="CUDA backend is available only in the Windows build";public:bool init(){return false;}bool ready()const{return false;}const std::string&device()const{return error_;}const std::string&error()const{return error_;}void train(Trainer&,const std::vector<Sample>&,int=1){}};}
#endif
