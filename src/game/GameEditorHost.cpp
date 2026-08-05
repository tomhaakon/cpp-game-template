#include "game/GameEditorHost.h"
#if TEYA_ENABLE_EDITOR
#include "game/Game.h"
#include "player/Player.h"
#include <teya/animation/AnimationIO.h>
#include <teya/core/AssetPath.h>
#include <teya/core/Profile.h>
#include <memory>
namespace { constexpr std::uint64_t PlayerAnimationId=1; }
RenderTexture2D GameEditorHost::gameViewTexture() const{return game_.editorTexture();}
int GameEditorHost::gameCanvasWidth() const{return game_.editorCanvasWidth();}
int GameEditorHost::gameCanvasHeight() const{return game_.editorCanvasHeight();}
std::vector<teya::editor::RuntimeNode> GameEditorHost::runtimeHierarchy() const{return game_.editorHierarchy();}
std::vector<teya::editor::RuntimeProperty> GameEditorHost::inspectObject(teya::editor::RuntimeObjectId id) const{return game_.editorProperties(id);}
teya::editor::EditorFrameMetrics GameEditorHost::frameMetrics() const{return game_.editorMetrics();}
std::vector<teya::editor::EditableAnimationAssetInfo> GameEditorHost::editableAnimationAssets() const{return {{PlayerAnimationId,"Player",game_.editorPlayer().animationAssetPath()}};}
teya::editor::AnimationWorkingCopyResult GameEditorHost::loadAnimationWorkingCopy(std::uint64_t id){auto&p=game_.editorPlayer();if(id!=PlayerAnimationId){teya::editor::AnimationWorkingCopyResult r;r.error="Unknown animation asset";return r;}if(!p.animationAsset()){teya::editor::AnimationWorkingCopyResult r;r.error="The player animation is not loaded";return r;}auto texture=p.animationTexture();return {p.animationAsset(),texture,texture.width,texture.height,{}};}
teya::animation::AnimationValidationResult GameEditorHost::validateEditableAnimation(const teya::animation::AnimationAsset&a,int w,int h)const{return teya::animation::validateAnimationAsset(a,w,h);}
teya::editor::AnimationSaveResult GameEditorHost::saveAndApplyAnimationAsset(std::uint64_t id,const teya::animation::AnimationAsset&a){TEYA_PROFILE_ZONE_NAMED("AnimationEditorHost::saveAndApply");if(id!=PlayerAnimationId)return {false,false,"Unknown animation asset"};auto validation=validateEditableAnimation(a,game_.editorPlayer().animationTexture().width,game_.editorPlayer().animationTexture().height);if(!validation.valid())return {false,false,"Animation validation failed"};std::string error;if(!teya::animation::saveAnimationAsset(teya::core::assets::path(game_.editorPlayer().animationAssetPath()),a,nullptr,&error))return {false,false,error};if(!game_.editorPlayer().applyAnimationAsset(std::make_shared<const teya::animation::AnimationAsset>(a),error))return {true,false,"Saved to disk, but live application failed: "+error};return {true,true,{}};}
teya::editor::AnimationSaveResult GameEditorHost::applyAnimationAssetWithoutSaving(std::uint64_t id,const teya::animation::AnimationAsset&a){TEYA_PROFILE_ZONE_NAMED("AnimationEditorHost::apply");if(id!=PlayerAnimationId)return {false,false,"Unknown animation asset"};auto validation=validateEditableAnimation(a,game_.editorPlayer().animationTexture().width,game_.editorPlayer().animationTexture().height);if(!validation.valid())return {false,false,"Animation validation failed"};std::string error;if(!game_.editorPlayer().applyAnimationAsset(std::make_shared<const teya::animation::AnimationAsset>(a),error))return {false,false,error};return {false,true,{}};}
std::vector<std::string> GameEditorHost::animationEventSuggestions()const{return {"attack_started","attack_active","spawn_slash","attack_finished","play_sound","footstep"};}
std::vector<std::string> GameEditorHost::animationMarkerTypeSuggestions()const{return {"effect","sound","footstep","interaction","camera"};}
void GameEditorHost::requestExit(){game_.requestEditorExit();}
#endif
