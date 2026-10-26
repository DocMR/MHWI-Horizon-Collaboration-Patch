#include "hooks.h"
#include "stormbolt_equipment_startup_gate.h"
#include "stormbolt_runtime_bridge.h"
#include "stormbolt_shooting_session.h"
#include "stormbolt_runtime_callback_readiness.h"
#include "stormbolt_runtime_dispatch_gate.h"
#include "stormbolt_charge_hud_native_input.h"
#include "stormbolt_hss_native_type.h"
#include "stormbolt_hss_catalog.h"
#include "stormbolt_shot_effects.h"
#if defined(STORMSLINGER_NATIVE_QUEST_SAVE)
#include "stormbolt_quest_mapping.h"
#endif
#if defined(STORMSLINGER_HORIZON_COMPLETE)
#include "../../staging/Horizon-complete-integration-20261021/src/horizon_complete_content_session.h"
#include "../../staging/Horizon-complete-integration-20261021/src/horizon_quest_catalog.h"
#endif
#include <array>
#include <cstring>
#include <cstdint>
#include <atomic>
#if defined(STORMSLINGER_STANDALONE_PROXY) || defined(STORMSLINGER_RUNTIME_DIAGNOSTICS)
#include "runtime_log.h"
#endif
#if defined(STORMSLINGER_WEAPON_TREE_FILTER)
#include "weapon_tree_filter_adapter.h"
#include "weapon_tree_clone.h"
#include "weapon_tree_code_guard.h"
#endif
struct QuestPlan { unsigned version; unsigned count; unsigned long long rva[12]; };
extern "C" __declspec(dllexport) int StormslingerStartQuestBridge(void*,const QuestPlan*);
#include "live_guard.inc"
// This bridge registers only the two project quests. Completed/progress getters
// remain untouched until genuine per-character persistence is implemented.
namespace {
stormslinger::StormboltEquipmentStartupGate equipmentBootstrap;
QuestPlan equipmentBootstrapPlan{};
void* equipmentBootstrapImage=nullptr;
std::size_t equipmentBootstrapImageBytes=0;
int StartEquipmentQuest(void*) {
#if defined(STORMSLINGER_DIAGNOSTIC_NO_SHOOTING)
 return StormslingerStartQuestBridge(equipmentBootstrapImage,&equipmentBootstrapPlan);
#else
 return stormslinger::stormbolt_start_native_callback_candidate(
     [] {return StormslingerStartQuestBridge(equipmentBootstrapImage,&equipmentBootstrapPlan);},
     [](const stormslinger::StormboltRuntimeReadiness& readiness) {
         return StormslingerStartShootingBridge(equipmentBootstrapImage,equipmentBootstrapImageBytes,&readiness);
     });
#endif
}
stormslinger::HookTransaction questTransaction;
stormslinger::HookTransaction shootingTransaction;
stormslinger::StormboltShootingSession<64> shootingSession;
#if defined(STORMSLINGER_HORIZON_COMPLETE)
std::optional<horizon_other::CompleteContentSession<64>> completeContent;
horizon_other::ModulePrepareResult completePrepared{};
#endif
stormslinger::StormboltSessionResult restoreShootingComponents() {
#if defined(STORMSLINGER_HORIZON_COMPLETE)
 if(shootingSession.state()==stormslinger::StormboltSessionState::fresh ||
    shootingSession.state()==stormslinger::StormboltSessionState::stopped)
  return {shootingSession.state(),stormslinger::StormboltSessionIssue::none};
 if(completeContent && !completeContent->restore_before_shared_stop())
  return {shootingSession.state(),stormslinger::StormboltSessionIssue::power_cell_restore};
 auto restored=shootingSession.restore_before_shared_stop_while_quiescent();
 if(!restored.ok())return restored;
 const auto status=shootingTransaction.stop();
 if(status!=MH_OK)return {shootingSession.state(),stormslinger::StormboltSessionIssue::hook_restore,status};
 if(completeContent && !completeContent->release_after_shared_stop())
  return {shootingSession.state(),stormslinger::StormboltSessionIssue::power_cell_restore};
 return shootingSession.release_after_shared_stop_while_quiescent();
#else
 return shootingSession.restore();
#endif
}
stormslinger::StormboltDefaultHeapBindings shootingHeap{};
void* shootingTransitionType=nullptr;
std::optional<stormslinger::StormboltRuntimeCallbackReadiness> shootingReadiness;
stormslinger::StormboltWeaponEffectDependencies shootingEffectDependencies{};
stormslinger::StormboltRuntimeDispatchGate shootingDispatch;
stormslinger::StormboltRuntimeReadiness shootingOverrides{};
stormslinger::StormboltChargeHudIdentity shootingHudIdentity{};
stormslinger::StormboltShotEffectApi shootingShotEffectApi{};
void** shootingHudPlayerManager=nullptr;
void* (*shootingHudLocalPlayer)(void*)=nullptr;
stormslinger::StormboltHssNativeType<64> shootingHssType;
stormslinger::StormboltHssCatalog shootingHssCatalog;
void* shootingHssLocalActor() {
 if(!shootingDispatch.weapon() || !shootingHudPlayerManager || !*shootingHudPlayerManager ||
     !shootingHudLocalPlayer) return nullptr;
 return shootingHudLocalPlayer(*shootingHudPlayerManager);
}
bool prepareShootingHssType(std::uintptr_t image) {
 if(shootingHssType.type()) return true; // Metadata remains registered for this process.
 stormslinger::StormboltHssTypeBindings api{};
 api.register_type=reinterpret_cast<decltype(api.register_type)>(image+0x216ED60);
 api.parent_type=reinterpret_cast<void*>(image+0x502CFF0);
 api.base_vtable=reinterpret_cast<void*>(image+0x3499028);
 api.dti_vtable=reinterpret_cast<void*>(image+0x346EEA0);
 api.construct_base=reinterpret_cast<decltype(api.construct_base)>(image+0x1FB3D10);
 api.destroy_base=reinterpret_cast<decltype(api.destroy_base)>(image+0x1FB4210);
 api.set_priority=reinterpret_cast<decltype(api.set_priority)>(image+0x1FB7DC0);
 api.request_show=reinterpret_cast<decltype(api.request_show)>(image+0x1FB7CF0);
 api.remove_gui=reinterpret_cast<decltype(api.remove_gui)>(image+0x2228F10);
 api.local_actor=&shootingHssLocalActor;api.heap=shootingHeap;
 api.gui.nodes={reinterpret_cast<decltype(api.gui.nodes.find_root)>(image+0x23537A0),
     reinterpret_cast<decltype(api.gui.nodes.find_child)>(image+0x2340E20)};
 api.gui.callbacks={reinterpret_cast<decltype(api.gui.callbacks.swap_function)>(image+0x24B980),
     reinterpret_cast<decltype(api.gui.callbacks.play_terminal)>(image+0x2342AF0)};
 api.gui.identity={shootingHudIdentity.actor,shootingHudIdentity.weapon};
 api.gui.input.action_filter=reinterpret_cast<decltype(api.gui.input.action_filter)>(image+0x1F5AE10);
 // PS3A64EF0 skips an already active animation. PC18C6140 restarts it;
 // use its native changed-only wrapper so per-frame stage refresh can finish.
 api.gui.animation={reinterpret_cast<decltype(api.gui.animation.animation)>(image+0x2340780),
     reinterpret_cast<decltype(api.gui.animation.play)>(image+0x18C6210)};
 api.gui.hide_gui=reinterpret_cast<decltype(api.gui.hide_gui)>(image+0x1FB7CC0);
 api.gui.hide_node=reinterpret_cast<decltype(api.gui.hide_node)>(image+0x2343310);
 api.gui.text_manager_slot=reinterpret_cast<void**>(image+0x5012DF0);
 api.gui.text_at=reinterpret_cast<decltype(api.gui.text_at)>(image+0x2244930);
 api.gui.fit_warning_text=reinterpret_cast<decltype(api.gui.fit_warning_text)>(image+0x18C1C50);
 return shootingHssType.register_type(api,shootingSession.instances());
}
void retireShootingHss() {
 shootingHssCatalog.withdraw();
 if(shootingHssType.type())shootingHssType.retire_instances();
}
bool shootingHintMode(bool& loaded) {
 if(!shootingDispatch.weapon() || !shootingHudPlayerManager || !*shootingHudPlayerManager ||
     !shootingHudLocalPlayer) return false;
 auto* actor=shootingHudLocalPlayer(*shootingHudPlayerManager);
 stormslinger::StormboltChargeHudInput input{};
 if(!stormslinger::stormbolt_charge_hud_actor_input(reinterpret_cast<std::uintptr_t>(actor),
     shootingHudIdentity,input)) return false;
 loaded=input.special_ammo_selected;
 return true;
}
bool shootingHudInput(void* controller,stormslinger::StormboltChargeHudInput& input) {
 return shootingDispatch.weapon() &&
     stormslinger::stormbolt_charge_hud_native_input(controller,shootingHudIdentity,input);
}
bool shootingItemWindowInput(void* window,stormslinger::StormboltChargeHudInput& input) {
 if(!shootingDispatch.weapon()) return false;
 const auto address=reinterpret_cast<std::uintptr_t>(window);
 std::uintptr_t actor=0;
 return stormslinger::read_stormbolt_field(address,0x4548,actor) &&
     stormslinger::stormbolt_charge_hud_actor_input(actor,shootingHudIdentity,input);
}
bool shootingOrdinaryReticleInput(void* gui,stormslinger::StormboltChargeHudInput& input) {
 if(!shootingDispatch.weapon())return false;
 std::uintptr_t actor=0;
 return stormslinger::read_stormbolt_field(reinterpret_cast<std::uintptr_t>(gui),0x28F0,actor) &&
     stormslinger::stormbolt_charge_hud_actor_input(actor,shootingHudIdentity,input);
}
float shootingHudFrame(void* node) {
 return stormslinger::stormbolt_source_field<float>(node,0x5C);
}
bool shootingDamageDispatchReady() {return shootingDispatch.weapon();}
bool shootingShotSelfDamage(void* weapon,std::uint32_t snapshot) {
 auto* power=shootingSession.power_cell();
 return shootingDispatch.weapon() && power && power->shot_self_damage(weapon,snapshot);
}
bool shootingShotEffects(void* weapon,std::uint32_t snapshot) {
 return shootingDispatch.weapon() && stormslinger::stormbolt_shot_effects(
     shootingSession.instances(),weapon,snapshot,shootingShotEffectApi);
}
bool shootingSpecialReady(void* action,stormslinger::StormboltSpecialActionPhase phase) {
 return shootingDispatch.action(phase) && (shootingOverrides.special?shootingOverrides.special(action,phase):
     shootingReadiness && shootingReadiness->action(action,phase,stormslinger::StormboltRuntimeAction::special));
}
bool shootingGatlingReady(void* action,stormslinger::StormboltSpecialActionPhase phase) {
 return shootingDispatch.action(phase) && (shootingOverrides.gatling?shootingOverrides.gatling(action,phase):
     shootingReadiness && shootingReadiness->action(action,phase,stormslinger::StormboltRuntimeAction::gatling));
}
bool shootingMiddleReady(void* action,stormslinger::StormboltSpecialActionPhase phase) {
 return shootingDispatch.action(phase) && (shootingOverrides.middle?shootingOverrides.middle(action,phase):
     shootingReadiness && shootingReadiness->action(action,phase,stormslinger::StormboltRuntimeAction::middle));
}
bool shootingPrimaryShootReady(void* action,stormslinger::StormboltSpecialActionPhase phase) {
 return shootingDispatch.action(phase) && shootingReadiness &&
     shootingReadiness->action(action,phase,stormslinger::StormboltRuntimeAction::primary_shoot);
}
bool shootingLastReady(void* action,stormslinger::StormboltSpecialActionPhase phase) {
 return shootingDispatch.action(phase) && (shootingOverrides.last?shootingOverrides.last(action,phase):
     shootingReadiness && shootingReadiness->action(action,phase,stormslinger::StormboltRuntimeAction::last));
}
bool shootingUpdateReady(void* weapon) {
 return shootingDispatch.weapon() && (shootingOverrides.update?shootingOverrides.update(weapon):
     shootingReadiness && shootingReadiness->weapon(weapon));
}
bool shootingConditionDispatchReady(void*) {return shootingDispatch.weapon();}
bool shootingResourcesReady(void* component,void* resource,std::uint32_t group,std::uint32_t record,void*) {
 if(!shootingDispatch.weapon()) return false;
 const auto& api=shootingOverrides.evade;
 const bool ready=api.resource_ready?api.resource_ready(component,resource,group,record,api.resource_context):
     stormslinger::StormboltWeaponEffectDependencies::callback(component,resource,group,record,&shootingEffectDependencies);
#if defined(STORMSLINGER_STANDALONE_PROXY) || defined(STORMSLINGER_RUNTIME_DIAGNOSTICS)
 if(!ready)stormslinger::runtime_log::resource_wait(group,record);
#endif
 return ready;
}
void* allocateShootingTransition(void* heap,std::size_t bytes,std::uint32_t alignment,std::uint32_t tag,
 std::uint32_t a,std::uint32_t b,const void* label) {
 return stormslinger::stormbolt_allocate_default_heap(shootingHeap,shootingTransitionType,
     heap,bytes,alignment,tag,a,b,label);
}
SRWLOCK control=SRWLOCK_INIT;
std::atomic<int> weaponPhaseBefore{-1},weaponPhaseAfter{-1};
#if defined(STORMSLINGER_WEAPON_TREE_FILTER)
stormslinger::TreeExcluded treeExcludedOriginal=nullptr;
stormslinger::NativeEquipLookup nativeEquipLookup=nullptr;
stormslinger::WeaponTreeClone weaponTreeClone;
void* treeBuilderOriginal=nullptr;
bool treeExcludedHook(void* context,const void* recipe) {
 return stormslinger::filterTreeExclusion(context,recipe,treeExcludedOriginal,nativeEquipLookup);
}
#endif
#if defined(STORMSLINGER_ORIGINAL_QUEST_IDS)
constexpr int questIds[]={67802,67803};
constexpr const char* questNames[]={"quest\\questData_67802","quest\\questData_67803"};
#else
constexpr int questIds[]={96782,96783};
constexpr const char* questNames[]={"quest\\questData_96782","quest\\questData_96783"};
#endif
bool custom(int id){return id==questIds[0] || id==questIds[1];}
using Count=int(*)();using At=int(*)(void*,int);using Master=int(*)(int);
using Category=long long(*)(int,int);using Star=bool(*)(int,int,int);
using Load=bool(*)(void*,void*);using Grow=void(*)(void*,uint32_t);
Count countOriginal=nullptr;At atOriginal=nullptr;Master masterOriginal=nullptr;
#if defined(STORMSLINGER_NATIVE_QUEST_SAVE)
stormslinger::StormboltQuestMapping nativeQuestMapping;
#endif
Category categoryOriginal=nullptr;Star starOriginal=nullptr;
Load dataOriginal=nullptr,listOriginal=nullptr;Grow grow=nullptr;
struct QuestNo {void* vtable;int id;unsigned padding;};
static_assert(sizeof(QuestNo)==0x10);
using AllocateNode=void*(*)(size_t,unsigned);
using ConstructNode=QuestNo*(*)(void*);
using DestroyNode=void*(*)(void*,unsigned);
AllocateNode allocateNode=nullptr;ConstructNode constructNode=nullptr;
void destroyNode(QuestNo* node){if(node)reinterpret_cast<DestroyNode>(**static_cast<void***>(static_cast<void*>(node)))(node,1);}
#if !defined(STORMSLINGER_NATIVE_QUEST_SAVE)
int countHook(){return countOriginal()+2;}
int atHook(void* object,int index){int n=countOriginal();return index>=n && index<n+2?questIds[index-n]:atOriginal(object,index);}
#endif
int masterHook(int id){
#if defined(STORMSLINGER_HORIZON_COMPLETE)
 if(const auto* quest=horizon_other::find_quest(id))return quest->source_rank==2;
#endif
 return custom(id)?1:masterOriginal(id);
}
long long categoryHook(int id,int unknown){return custom(id)?1:categoryOriginal(id,unknown);}
bool starHook(int id,int category,int star){
#if defined(STORMSLINGER_HORIZON_COMPLETE)
 if(const auto* quest=horizon_other::find_quest(id))return category==1 && star==quest->star;
#else
 if(custom(id))return category==1 && star==16;
#if defined(STORMSLINGER_NATIVE_QUEST_SAVE)
 // Shared save-index layout is not an acquisition catalog. This two-quest
 // build must not publish the other module's four unavailable task entries.
 if(id==67101 || id==67102 || id==67607 || id==67804)return false;
#endif
#endif
 return starOriginal(id,category,star);
}
bool dataHook(void* object,void* stream){
 bool loaded=dataOriginal(object,stream);if(!loaded || !object)return loaded;
 const auto name=reinterpret_cast<const char*>(object)+0xc;
 for(size_t i=0;i<2;++i){const char* expected=questNames[i];
  if(!strcmp(name,expected)){*reinterpret_cast<int*>(static_cast<char*>(object)+0xe0)=questIds[i];break;}}
 return loaded;
}
bool listHook(void* object,void* stream){
 bool loaded=listOriginal(object,stream);if(!loaded || !object)return loaded;
 const char* name=reinterpret_cast<const char*>(object)+0xc;
 if(strcmp(name,"quest\\questNoList"))return loaded;
 auto n=reinterpret_cast<int*>(static_cast<char*>(object)+0xb8);
 auto capacity=reinterpret_cast<int*>(static_cast<char*>(object)+0xbc);
 auto entries=reinterpret_cast<void***>(static_cast<char*>(object)+0xc8);
 if(*n<0 || *capacity<*n || *capacity>100000 || !*entries)return loaded;
 // Native allocations are owned by the resource, including during reloads.
 bool missing[2]={true,true};
 for(int j=0;j<*n;++j){auto node=static_cast<const QuestNo*>((*entries)[j]);
  if(node)for(size_t i=0;i<2;++i)if(node->id==questIds[i])missing[i]=false;}
 unsigned needed=unsigned(missing[0])+unsigned(missing[1]);
 if(!needed)return loaded;
 grow(static_cast<char*>(object)+0xb0,static_cast<uint32_t>(*capacity+needed));
 if(!*entries || *capacity<*n+static_cast<int>(needed))return loaded;
 QuestNo* pending[2]={nullptr,nullptr};
 for(size_t i=0;i<2;++i)if(missing[i]){
  void* memory=allocateNode(sizeof(QuestNo),0x10);
  if(!memory){for(auto node:pending)destroyNode(node);return loaded;}
  pending[i]=constructNode(memory);pending[i]->id=questIds[i];
 }
 for(auto node:pending)if(node)(*entries)[(*n)++]=node;
 return loaded;
}
}
// Caller must have passed both the executable hash gate and a fresh live-code
// plan. This entry is for development integration; no automatic startup yet.
extern "C" __declspec(dllexport) int StormslingerStartQuestBridge(void* imageBase,const QuestPlan* plan){
 if(!imageBase || imageBase!=GetModuleHandleW(nullptr) || !plan || plan->version!=1 || plan->count!=12)return -1;
 const unsigned expected[]={0xf2c320,0x1976110,0x19751b0,0x249b70,0x1b659b0,0x19750c0,0x478a20,0x1a10170,0x134ef50,0x134efe0,0x1355c50,0x1355c40};
 for(size_t i=0;i<12;++i)if(plan->rva[i]!=expected[i])return -2;
 int guard=ValidateCurrentGame(imageBase,plan);if(guard)return guard;
 auto image=static_cast<const unsigned char*>(imageBase);
#if defined(STORMSLINGER_WEAPON_TREE_FILTER)
 if(!stormslinger::validateWeaponTreeCode(image))return -17;
 // The native resource loader starts tree construction in phase4. Reject a
 // late bootstrap rather than report success with already-filtered nodes.
 void* equipmentManager=nullptr;SIZE_T read=0;int equipmentPhase=-1;
 if(!ReadProcessMemory(GetCurrentProcess(),image+0x5011f00,&equipmentManager,sizeof(equipmentManager),&read) || read!=sizeof(equipmentManager) || !equipmentManager)return -13;
 if(!ReadProcessMemory(GetCurrentProcess(),static_cast<unsigned char*>(equipmentManager)+0x38,&equipmentPhase,sizeof(equipmentPhase),&read) || read!=sizeof(equipmentPhase))return -19;
 weaponPhaseBefore.store(equipmentPhase);
 if(equipmentPhase<0 || equipmentPhase>=4)return -19;
#endif
 const unsigned char ctorPrefix[]={0x48,0x8d,0x05,0x91,0x86,0xbf,0x02,0xc7,0x41,0x08,0,0,0,0};
 const unsigned char allocatorPrefix[]={0x48,0x89,0x5c,0x24,0x08,0x57,0x48,0x83,0xec,0x40};
 if(memcmp(image+0x32a330,ctorPrefix,sizeof(ctorPrefix)) ||
    memcmp(image+0x32a350,allocatorPrefix,sizeof(allocatorPrefix)))return -16;
 AcquireSRWLockExclusive(&control);
 auto status=equipmentBootstrap.armed()?questTransaction.begin_shared_with(equipmentBootstrap.transaction()):questTransaction.begin();
 if(status==MH_OK){
  HMODULE keepAlive=nullptr;
  if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,reinterpret_cast<LPCWSTR>(&StormslingerStartQuestBridge),&keepAlive)){questTransaction.stop();ReleaseSRWLockExclusive(&control);return -15;}
  auto base=static_cast<char*>(imageBase);grow=reinterpret_cast<Grow>(base+plan->rva[3]);
#if defined(STORMSLINGER_WEAPON_TREE_FILTER)
#if !defined(STORMSLINGER_HORIZON_COMPLETE)
  if(!weaponTreeClone.prepare(image)) {
   questTransaction.stop();ReleaseSRWLockExclusive(&control);return -18;
  }
#endif
#endif
  allocateNode=reinterpret_cast<AllocateNode>(base+0x32a350);
  constructNode=reinterpret_cast<ConstructNode>(base+0x32a330);
  struct Spec{size_t index;void* hook;void** original;};
  Spec specs[]={
#if !defined(STORMSLINGER_NATIVE_QUEST_SAVE)
   {10,reinterpret_cast<void*>(countHook),reinterpret_cast<void**>(&countOriginal)},
   {11,reinterpret_cast<void*>(atHook),reinterpret_cast<void**>(&atOriginal)},
#endif
   {1,reinterpret_cast<void*>(masterHook),reinterpret_cast<void**>(&masterOriginal)},
   {0,reinterpret_cast<void*>(starHook),reinterpret_cast<void**>(&starOriginal)},
#if !defined(STORMSLINGER_HORIZON_COMPLETE)
   {2,reinterpret_cast<void*>(categoryHook),reinterpret_cast<void**>(&categoryOriginal)},
   {7,reinterpret_cast<void*>(dataHook),reinterpret_cast<void**>(&dataOriginal)},
   {6,reinterpret_cast<void*>(listHook),reinterpret_cast<void**>(&listOriginal)},
#endif
  };
  for(const auto& spec:specs){status=questTransaction.add(base+plan->rva[spec.index],spec.hook,spec.original);if(status!=MH_OK)break;}
#if defined(STORMSLINGER_NATIVE_QUEST_SAVE)
  if(status==MH_OK) {
#if defined(STORMSLINGER_COMPATIBLE_PLUGIN)
   if(!nativeQuestMapping.prepare(image,AuthenticatedQuestLoader())) status=MH_ERROR_UNSUPPORTED_FUNCTION;
#else
   if(!nativeQuestMapping.prepare(image)) status=MH_ERROR_UNSUPPORTED_FUNCTION;
#endif
   else status=nativeQuestMapping.queue(questTransaction);
  }
#endif
#if defined(STORMSLINGER_WEAPON_TREE_FILTER) && !defined(STORMSLINGER_HORIZON_COMPLETE)
  if(status==MH_OK) {
   nativeEquipLookup=reinterpret_cast<stormslinger::NativeEquipLookup>(base+0x197e8a0);
   status=questTransaction.add(base+0x13738b0,reinterpret_cast<void*>(treeExcludedHook),reinterpret_cast<void**>(&treeExcludedOriginal));
  }
  if(status==MH_OK)status=questTransaction.add(base+0x13732f0,weaponTreeClone.entry(),&treeBuilderOriginal);
#endif
#if defined(STORMSLINGER_HORIZON_COMPLETE)
  // Queue the module's original-owner hooks before publishing the shared RIP
  // table: its getter owner authentication includes those original displacements.
  if(status==MH_OK) {
   completePrepared=horizon_other::prepare_horizon_other_hooks(questTransaction,image,nullptr,&nativeQuestMapping.prepared());
   if(completePrepared.stage!=horizon_other::ModulePrepareStage::queued)status=MH_ERROR_UNSUPPORTED_FUNCTION;
  }
#endif
  if(status==MH_OK)status=questTransaction.commit();
#if defined(STORMSLINGER_WEAPON_TREE_FILTER)
  if(status==MH_OK) {
   void* managerAfter=nullptr;int phaseAfter=-1;SIZE_T count=0;
   bool early=ReadProcessMemory(GetCurrentProcess(),image+0x5011f00,&managerAfter,sizeof(managerAfter),&count) && count==sizeof(managerAfter) && managerAfter==equipmentManager;
   if(early)early=ReadProcessMemory(GetCurrentProcess(),static_cast<unsigned char*>(managerAfter)+0x38,&phaseAfter,sizeof(phaseAfter),&count) && count==sizeof(phaseAfter) && phaseAfter>=0 && phaseAfter<4;
   weaponPhaseAfter.store(phaseAfter);
   if(!early) {
    auto rollback=questTransaction.stop();ReleaseSRWLockExclusive(&control);
    return rollback==MH_OK?-20:-21;
   }
  }
#endif
 }
 ReleaseSRWLockExclusive(&control);return static_cast<int>(status);
}
// Arm from the existing proxy worker; initialization runs on the NEXT typed
// native equipment phase entry, before its original stage dispatch executes.
// Candidate relies on the user-selected native callback lifetime convention;
// equipment phase alone is not proof of global quiescence or pre-birth.
extern "C" int StormslingerArmEquipmentBootstrap(void* imageBase,const QuestPlan* plan) {
 if(equipmentBootstrap.armed()) return 0;
 if(!imageBase || imageBase!=GetModuleHandleW(nullptr) || !plan || plan->version!=1 || plan->count!=12) return -1;
 const int guard=ValidateCurrentGame(imageBase,plan);if(guard) return guard;
 const auto* image=static_cast<const unsigned char*>(imageBase);
#if defined(STORMSLINGER_STANDALONE_PROXY) || defined(STORMSLINGER_RUNTIME_DIAGNOSTICS)
 // The executable hash gate above authenticates this import slot. Observe
 // only game-originated ExitProcess requests; do not hook the process globally.
 stormslinger::runtime_log::observe_exit_slot(reinterpret_cast<void**>(static_cast<unsigned char*>(imageBase)+0x2e13260));
 // The game's CRT exit path calls TerminateProcess on itself before ExitProcess.
 stormslinger::runtime_log::observe_terminate_slot(reinterpret_cast<void**>(static_cast<unsigned char*>(imageBase)+0x2e13490));
#endif
 const unsigned char expected[]={0x48,0x89,0x5c,0x24,0x10,0x48,0x89,0x4c,0x24,0x08,0x55,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57,0x48,0x8b,0xec,0x48,0x81,0xec,0x80,0,0,0,0x8b};
 const unsigned char dispatch[]={0x8b,0x41,0x38,0x48,0x8b,0xf9,0x83,0xf8,0x0b,0x77,0x1c,0x48,0x8d,0x0d,0xef,0x3f,0x48,0xfe,0x44,0x8b,0x84,0x81,0x54,0xc7,0xb7,0x01,0x4c,0x03,0xc1,0x41,0xff,0xe0};
 unsigned char live[32]{};SIZE_T count=0;
 if(!ReadProcessMemory(GetCurrentProcess(),image+0x1b7bfe0,live,sizeof(live),&count) || count!=sizeof(live) || memcmp(live,expected,sizeof(live))) return -22;
 if(!ReadProcessMemory(GetCurrentProcess(),image+0x1b7bfff,live,sizeof(live),&count) || count!=sizeof(live) || memcmp(live,dispatch,sizeof(live))) return -22;
 AcquireSRWLockExclusive(&control);
 equipmentBootstrapImage=imageBase;equipmentBootstrapPlan=*plan;
 const auto* dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(imageBase);
 const auto* headers=reinterpret_cast<const IMAGE_NT_HEADERS64*>(image+dos->e_lfanew);
 equipmentBootstrapImageBytes=headers->OptionalHeader.SizeOfImage;
 HMODULE keepAlive=nullptr;
 if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
     reinterpret_cast<LPCWSTR>(&StormslingerArmEquipmentBootstrap),&keepAlive)) {
  ReleaseSRWLockExclusive(&control);return -15;
 }
 auto status=equipmentBootstrap.arm(reinterpret_cast<stormslinger::StormboltEquipmentStartupGate::Native>(static_cast<char*>(imageBase)+0x1b7bfe0),
     reinterpret_cast<void**>(static_cast<char*>(imageBase)+0x5011f00),
     reinterpret_cast<std::uintptr_t>(imageBase)+0x33ff428,&StartEquipmentQuest,nullptr);
 ReleaseSRWLockExclusive(&control);
 if(status!=MH_OK && equipmentBootstrap.result()==-19) return -19;
 return status==MH_OK?0:-500-static_cast<int>(status);
}
extern "C" void StormslingerEquipmentBootstrapDiagnostics(int* stage,int* status,DWORD* origin,DWORD* error) {
 if(stage)*stage=equipmentBootstrap.install_stage();
 if(status)*status=equipmentBootstrap.install_status();
 if(origin)*origin=equipmentBootstrap.failure_origin();
 if(error)*error=equipmentBootstrap.failure_error();
}
extern "C" int StormslingerEquipmentBootstrapResult() {return equipmentBootstrap.result();}
extern "C" int StormslingerCancelEquipmentBootstrap() {return equipmentBootstrap.cancel_pending(-30);}
extern "C" __declspec(dllexport) int StormslingerStopQuestBridge(){
 AcquireSRWLockExclusive(&control);
 shootingDispatch.begin_restore();
 retireShootingHss();
 const auto restored=restoreShootingComponents();
 if(!restored.ok()) {ReleaseSRWLockExclusive(&control);return -400-static_cast<int>(restored.issue);}
 auto status=shootingTransaction.stop();
 if(status==MH_OK) {shootingDispatch.finish_restore();status=questTransaction.stop();}
 ReleaseSRWLockExclusive(&control);return static_cast<int>(status);
}
extern "C" __declspec(dllexport) int StormslingerStartShootingBridge(void* imageBase,std::size_t imageBytes,
 const stormslinger::StormboltRuntimeReadiness* readiness) {
 if(!imageBase || imageBase!=GetModuleHandleW(nullptr) || !readiness || readiness->version!=1 ||
    !readiness->conditions.synchronous_borrow_verified ||
    !readiness->evade.synchronous_borrow_verified) return -1;
 AcquireSRWLockExclusive(&control);
 struct Unlock {~Unlock(){ReleaseSRWLockExclusive(&control);}} unlock;
 if(shootingSession.state()!=stormslinger::StormboltSessionState::fresh || shootingTransaction.has_library_state()) return -2;
 stormslinger::StormboltNativeBindings bindings{};
 const auto checked=stormslinger::bind_stormbolt_native_code(reinterpret_cast<std::uintptr_t>(imageBase),imageBytes,bindings);
 if(checked.status!=stormslinger::StormboltCodeStatus::matched) return -100-static_cast<int>(checked.status);
#if defined(STORMSLINGER_HORIZON_COMPLETE)
 bindings.loop_inputs.target_weapon_vtable=0;
#endif
 const auto requestedAllocate=readiness->allocate;
 auto configured=stormslinger::stormbolt_configure_runtime_readiness(bindings,*readiness);
 readiness=&configured;
 stormslinger::StormboltDefaultHeapSelection sourceHeap{},conditionHeap{};
 if(!stormslinger::stormbolt_select_default_heap(bindings.default_heap,bindings.source_transition_type,sourceHeap) ||
    !stormslinger::stormbolt_select_default_heap(bindings.default_heap,bindings.shoot_condition.type,conditionHeap) ||
    (requestedAllocate && reinterpret_cast<std::uintptr_t>(requestedAllocate)!=sourceHeap.allocate)) return -4;
 configured.allocate=reinterpret_cast<decltype(configured.allocate)>(sourceHeap.allocate);
 bindings.shell.dispatch_ready=&shootingDamageDispatchReady;
 bindings.shell.before_energy=&shootingShotSelfDamage;
 shootingShotEffectApi={bindings.decay_effect,bindings.cleanup_bank,bindings.decay_owner,
     bindings.effect_color,bindings.remote_attachment,readiness->color};
 bindings.shell.after_energy=&shootingShotEffects;
 bindings.coefficients.dispatch_ready=&shootingDamageDispatchReady;
 bindings.normal_to_special.dispatch_ready=&shootingDamageDispatchReady;
 shootingOverrides=*readiness;
 shootingEffectDependencies={bindings.evade_epv_resource_vtable,bindings.evade_efx_resource_vtable};
 auto evadeReadiness=readiness->evade;
 evadeReadiness.resource_ready=&shootingResourcesReady;evadeReadiness.resource_context=nullptr;
 shootingReadiness.emplace(bindings,evadeReadiness,readiness->conditions.synchronous_borrow_verified);
 bindings.special_to_normal.hook.ready=&shootingSpecialReady;
 bindings.gatling_hook.ready=&shootingGatlingReady;bindings.middle_hook.ready=&shootingMiddleReady;
 bindings.primary_shoot_hook.ready=&shootingPrimaryShootReady;
 bindings.last_hook.ready=&shootingLastReady;bindings.weapon_update.ready=&shootingUpdateReady;
 auto status=shootingTransaction.begin_shared_with(questTransaction,true);
 if(status!=MH_OK) return static_cast<int>(status);
 shootingHeap=bindings.default_heap;shootingTransitionType=bindings.source_transition_type;
 unsigned startupComponent=1;
 auto fail=[&startupComponent](stormslinger::StormboltSessionResult result) {
#if defined(STORMSLINGER_HORIZON_STRACKER)
  stormslinger::runtime_log::startup_component(startupComponent,static_cast<unsigned>(result.issue),static_cast<int>(result.native_status));
#endif
  shootingDispatch.begin_restore();
  retireShootingHss();
  const auto restored=restoreShootingComponents();
  if(!restored.ok()) return -400-static_cast<int>(restored.issue);
  const auto stopped=shootingTransaction.stop();
  if(stopped!=MH_OK) return static_cast<int>(stopped);
  shootingDispatch.finish_restore();
  return -200-static_cast<int>(result.issue);
 };
 try {
  std::array<unsigned char,7> statusExpected{},timerExpected{};
  if(!stormslinger::read_stormbolt_field(bindings.power_cell_status_update_site,0,statusExpected) ||
     !stormslinger::read_stormbolt_field(bindings.power_cell_timer_update_site,0,timerExpected)) {
   shootingTransaction.stop();return -3;
  }
  auto result=shootingSession.prepare_with_update(shootingTransaction,bindings,readiness->color,readiness->low,readiness->high);
  if(!result.ok()) return fail(result);
#if !defined(STORMSLINGER_DIAGNOSTIC_NO_POWER_CELL)
  startupComponent=2;
  result=shootingSession.attach_power_cell(bindings,statusExpected,timerExpected,
      stormslinger::stormbolt_remote_update_bindings(bindings),stormslinger::stormbolt_sync_pack_bindings(bindings),
      bindings.power_cell_growth_sites,stormslinger::stormbolt_complete_growth_bindings(bindings));
  if(!result.ok()) return fail(result);
#endif
  startupComponent=3;
  result=shootingSession.attach_source_transition(bindings,&allocateShootingTransition);
  if(!result.ok()) return fail(result);
  startupComponent=4;
  result=shootingSession.attach_native_source_conditions(bindings,readiness->conditions,&shootingConditionDispatchReady);
  if(!result.ok()) return fail(result);
  startupComponent=5;
  result=shootingSession.attach_native_evade_effect(bindings,evadeReadiness,&shootingConditionDispatchReady);
  if(!result.ok()) return fail(result);
  const auto hudImage=reinterpret_cast<std::uintptr_t>(imageBase);
  startupComponent=6;
  result=shootingSession.attach_swap_energy({
      reinterpret_cast<void* (*)(void*)>(hudImage+0xAE7000),
      reinterpret_cast<void (*)(void*)>(hudImage+0xAE7120),
      reinterpret_cast<void* (*)(void*,std::uint32_t)>(hudImage+0xAE70C0),
      reinterpret_cast<void (*)(void*,float)>(hudImage+0xAE7170),
      reinterpret_cast<void (*)(void*)>(hudImage+0x2148780),
      reinterpret_cast<void (*)(void*)>(hudImage+0x21488C0),
      reinterpret_cast<float (*)(void*)>(hudImage+0x11C3420),
      hudImage+0x30AEC98,bindings.shell.weapon_vtable,bindings.coefficients.parameter_vtable});
  if(!result.ok()) return fail(result);
  startupComponent=7;
  result=shootingSession.attach_muzzle_effect({
      reinterpret_cast<stormslinger::StormboltMuzzleEffect>(hudImage+0xAE27E0),hudImage+0x214CE6B,
      bindings.normal_to_special.actor_vtable,bindings.shell.weapon_vtable,
      &shootingConditionDispatchReady,nullptr});
  if(!result.ok()) return fail(result);
  startupComponent=8;
  result=shootingSession.attach_motion_events({
      reinterpret_cast<stormslinger::StormboltTimelineBind>(hudImage+0x24A7F50),
      bindings.normal_to_special.actor_vtable,bindings.shell.weapon_vtable,
      &shootingConditionDispatchReady,nullptr});
  if(!result.ok()) return fail(result);
  shootingHudIdentity={hudImage+0x30C9768,bindings.normal_to_special.actor_vtable,
      bindings.shell.weapon_vtable,bindings.coefficients.parameter_vtable,bindings.update_gate};
  stormslinger::StormboltChargeHudBindings hud{};
  hud.controller_vtable=shootingHudIdentity.controller;
  hud.slot=reinterpret_cast<void*>(hud.controller_vtable+0x60);
  hud.original=reinterpret_cast<decltype(hud.original)>(hudImage+0xB6B680);
  hud.input=&shootingHudInput;
  hud.api={reinterpret_cast<decltype(hud.api.set_hidden)>(hudImage+0x2343310),
      reinterpret_cast<decltype(hud.api.set_frame)>(hudImage+0x2342D00),&shootingHudFrame,
      reinterpret_cast<decltype(hud.api.animation)>(hudImage+0x2340780),
      reinterpret_cast<decltype(hud.api.play)>(hudImage+0x18C6210)};
  startupComponent=9;
  result=shootingSession.attach_charge_hud(hud);
  if(!result.ok()) return fail(result);
  shootingHudPlayerManager=bindings.gatling_entry.player_manager_slot;
  shootingHudLocalPlayer=bindings.gatling_entry.local_player;
  startupComponent=10;
  result=shootingSession.attach_operation_hint({hudImage+0x3473360,
      reinterpret_cast<void(*)(void*,void*)>(hudImage+0xB97840),&shootingHintMode,
      hudImage+0x3473430,reinterpret_cast<void(*)(void*)>(hudImage+0x1E38AE0)});
  if(!result.ok()) return fail(result);
  startupComponent=11;
  result=shootingSession.attach_training_hint({hudImage+0x3215F10,
      reinterpret_cast<void(*)(void*,void*)>(hudImage+0xB97840),&shootingHintMode,
      hudImage+0x32161A0,reinterpret_cast<void(*)(void*)>(hudImage+0x10B0ED0)});
  if(!result.ok()) return fail(result);
  startupComponent=12;
  result=shootingSession.attach_itemwindow({hudImage+0x3471BE0,
      reinterpret_cast<void(*)(void*)>(hudImage+0x1E2B890),&shootingItemWindowInput,hud.api});
  if(!result.ok()) return fail(result);
  startupComponent=13;
  result=shootingSession.attach_itemwindow_mode(reinterpret_cast<void*>(hudImage+0x1E2D303));
  if(!result.ok()) return fail(result);
  startupComponent=14;
  result=shootingSession.attach_ordinary_reticle({hudImage+0x346EEC0,
      reinterpret_cast<bool(*)(void*)>(hudImage+0x1E14930),&shootingOrdinaryReticleInput});
  if(!result.ok())return fail(result);
  startupComponent=18;
  if(!prepareShootingHssType(hudImage))
      return fail({shootingSession.state(),stormslinger::StormboltSessionIssue::invalid_bindings,MH_OK});
  startupComponent=19;
  const auto hssCatalogStatus=shootingHssCatalog.prepare(shootingTransaction,hudImage,shootingHssType.type());
  if(hssCatalogStatus!=MH_OK)
      return fail({shootingSession.state(),stormslinger::StormboltSessionIssue::invalid_bindings,hssCatalogStatus});
#if defined(STORMSLINGER_HORIZON_COMPLETE)
  auto* shared=shootingSession.power_cell();
  if(!shared)return fail({shootingSession.state(),stormslinger::StormboltSessionIssue::invalid_bindings});
  completeContent.emplace(static_cast<const std::uint8_t*>(imageBase),shared->statuses(),bindings.loop_inputs,bindings.skill_value_query);
  startupComponent=20;
  if(!completeContent->prepare(shootingTransaction,completePrepared))
   return fail({shootingSession.state(),stormslinger::StormboltSessionIssue::invalid_bindings});
#endif
  startupComponent=21;
  result=shootingSession.activate();
  if(!result.ok()) return fail(result);
#if defined(STORMSLINGER_HORIZON_COMPLETE)
  startupComponent=22;
  if(!completeContent->activate_after_shared_commit())
   return fail({shootingSession.state(),stormslinger::StormboltSessionIssue::invalid_bindings});
#endif
  shootingDispatch.enable();
  return 0;
 } catch(const std::bad_alloc&) {
  return fail({shootingSession.state(),stormslinger::StormboltSessionIssue::invalid_bindings,MH_ERROR_MEMORY_ALLOC});
 }
}
extern "C" __declspec(dllexport) int StormslingerStopShootingBridge() {
 AcquireSRWLockExclusive(&control);
 shootingDispatch.begin_restore();
 retireShootingHss();
 const auto restored=restoreShootingComponents();
 const auto status=restored.ok()?static_cast<int>(shootingTransaction.stop()):-400-static_cast<int>(restored.issue);
 if(status==0) shootingDispatch.finish_restore();
 ReleaseSRWLockExclusive(&control);return status;
}
extern "C" int StormslingerWeaponTreeDiagnostics(int* before,int* after) {
 if(before)*before=weaponPhaseBefore.load();if(after)*after=weaponPhaseAfter.load();
#if defined(STORMSLINGER_WEAPON_TREE_FILTER)
 return 1;
#else
 return 0;
#endif
}


