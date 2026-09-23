#include "../src/v3_train.hpp"
#include <iostream>

using namespace aa;

int main(){
 try{
  std::mt19937 rng(7123);Game game(rng,0);InformationState information(game);V3Trainer trainer(1234);auto history=information.view(game.actor());auto target=makeBeliefTarget(game,game.actor());
  auto first=trainer.current.infer(history,trainer.config);float handSum=std::accumulate(first.opponentHand.begin(),first.opponentHand.end(),0.f);require(std::abs(handSum-1.f)<1e-5f,"Hand belief not normalized");
  auto actions=legal(history.events.back().observation);float policySum=0;for(int action:actions)policySum+=first.policy[action];require(std::abs(policySum-1.f)<1e-5f,"Policy not normalized");

  auto gruBefore=trainer.current.weights[v3Layout().wz];V3Sample sample{history,target,actions.front(),game.actor(),1.f};for(int i=0;i<8;i++)trainer.accumulate(sample);trainer.update();require(trainer.current.weights[v3Layout().wz]!=gruBefore,"GRU weights did not train");

  Game twin=game;std::reverse(twin.deck.begin(),twin.deck.end());for(int k=0;k<K;k++)std::swap(twin.hand[1][k],twin.hand[1][(k+1)%K]);
  // Restore conservation is irrelevant to the boundary: observation intentionally sees only hand size.
  InformationState twinInformation(twin);require(information.view(0)==twinInformation.view(0),"Hidden state changed player history");auto twinOutput=trainer.current.infer(twinInformation.view(0),trainer.config);auto output=trainer.current.infer(information.view(0),trainer.config);require(output.policy==twinOutput.policy,"Hidden state changed policy");

  for(int i=0;i<20;i++)trainer.episode();require(trainer.games==20&&trainer.updates>0,"V3 smoke training did not update");trainer.record({200,101,82,63,112});require(trainer.history.size()==1&&trainer.history.back().rates[2]==63.f/200&&trainer.history.back().exactCounts,"V3.1 evaluation record missing");auto migrated=unpackV3(packV3(trainer,3));require(migrated.history.size()==1&&migrated.history[0].rates[0]==101.f/200&&migrated.history[0].rates[2]<0&&!migrated.history[0].exactCounts,"V3.0 checkpoint history migration failed");
  auto directory=std::filesystem::temp_directory_path()/"agent-avenue-v3-checks";auto path=directory/"v3.bin";saveV3(trainer,path);require(checkpointKind(path)==CheckpointKind::RecurrentBeliefV3,"V3 checkpoint kind not recognized");auto restored=loadV3(path);require(packV3(restored)==packV3(trainer),"V3 checkpoint exact resume failed");
  auto resumedBefore=restored.games;restored.episode();require(restored.games==resumedBefore+1,"V3 resume did not continue");saveV3(restored,path);require(std::filesystem::exists(path.string()+".bak"),"V3 backup missing");
  auto restoredInference=loadV3(path.string()+".bak").current.infer(history,trainer.config);require(restoredInference.policy==trainer.current.infer(history,trainer.config).policy,"Checkpoint changed recomputed recurrent inference state");
  {std::fstream file(path,std::ios::binary|std::ios::in|std::ios::out);file.seekp(40);char zeros[16]{};file.write(zeros,16);}bool rejected=false;try{loadV3(path);}catch(...){rejected=true;}require(rejected,"Damaged v3 checkpoint accepted");require(packV3(loadV3(path.string()+".bak"))==packV3(trainer),"V3 backup recovery mismatch");

  V3Config recurrentOnly;recurrentOnly.beliefAuxiliary=false;recurrentOnly.beliefConditioned=false;V3Trainer ablation(88,recurrentOnly);ablation.episode();require(ablation.games==1,"Recurrent-only ablation failed");
  V3Config beliefOnly;beliefOnly.recurrent=false;beliefOnly.beliefAuxiliary=true;beliefOnly.beliefConditioned=false;V3Trainer beliefAblation(89,beliefOnly);beliefAblation.episode();require(beliefAblation.games==1,"Belief-only ablation failed");

  V3Trainer parallelA(77),parallelB(77);auto batchA=collectV3Batch(parallelA,parallelA.rng,16,4),batchB=collectV3Batch(parallelB,parallelB.rng,16,4);trainV3Batch(parallelA,std::move(batchA));trainV3Batch(parallelB,std::move(batchB));require(packV3(parallelA)==packV3(parallelB),"Parallel v3 batch is not deterministic");

  std::cout<<"PASS: GRU BPTT, constrained beliefs, belief-conditioned policy, ablations, deterministic parallel training, typed checkpoint, exact resume, corruption rejection and backup recovery. parameters="<<v3Layout().total<<"\n";return 0;
 }catch(const std::exception& error){std::cerr<<"FAIL: "<<error.what()<<"\n";return 1;}
}
