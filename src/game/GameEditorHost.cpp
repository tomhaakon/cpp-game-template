#include "game/GameEditorHost.h"
#if TEYA_ENABLE_EDITOR
#include "game/Game.h"
RenderTexture2D GameEditorHost::gameViewTexture() const{return game_.editorTexture();}
int GameEditorHost::gameCanvasWidth() const{return game_.editorCanvasWidth();}
int GameEditorHost::gameCanvasHeight() const{return game_.editorCanvasHeight();}
std::vector<teya::editor::RuntimeNode> GameEditorHost::runtimeHierarchy() const{return game_.editorHierarchy();}
std::vector<teya::editor::RuntimeProperty> GameEditorHost::inspectObject(teya::editor::RuntimeObjectId id) const{return game_.editorProperties(id);}
teya::editor::EditorFrameMetrics GameEditorHost::frameMetrics() const{return game_.editorMetrics();}
void GameEditorHost::requestExit(){game_.requestEditorExit();}
#endif
