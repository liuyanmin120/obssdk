#pragma once

#include <obs.hpp>
#include <graphics/vec2.h>
#include <graphics/matrix4.h>
#include "IBroadcastEngine.h"

#include <list>
#include <thread>
//#include "qt-display.hpp"
//#include "obs-app.hpp"

class SourceManager;
class CCEngine;
//class QMouseEvent;

#define ITEM_LEFT   (1<<0)
#define ITEM_RIGHT  (1<<1)
#define ITEM_TOP    (1<<2)
#define ITEM_BOTTOM (1<<3)

enum class ItemHandle : uint32_t {
	None         = 0,
	TopLeft      = ITEM_TOP | ITEM_LEFT,
	TopCenter    = ITEM_TOP,
	TopRight     = ITEM_TOP | ITEM_RIGHT,
	CenterLeft   = ITEM_LEFT,
	CenterRight  = ITEM_RIGHT,
	BottomLeft   = ITEM_BOTTOM | ITEM_LEFT,
	BottomCenter = ITEM_BOTTOM,
	BottomRight  = ITEM_BOTTOM | ITEM_RIGHT
};

class ScenePreview  : public IScenePreview
{

	typedef struct _GeometryInfo
	{
		int x;
		int y;
		int window_width;
		int window_height;
		float toBaseScale;
	}GeometryInfo;
public:
    Broadcast::EKey modifiers;

public:
	ScenePreview(std::shared_ptr<SourceManager>);
	~ScenePreview();

	const char *mousePressEvent(Broadcast::SMouseEvent *event) override;
	void mouseReleaseEvent(Broadcast::SMouseEvent *event) override;
	void mouseMoveEvent(Broadcast::SMouseEvent *event) override;
	
	void setRenderWindow(void *window) {
#ifdef WIN32
        m_window.hwnd = window;
#else
        m_window.view = (__unsafe_unretained id &)window;
#endif
    }
	void update() override;

	void bindScene(const char * scene_name) override;

	void * renderWindow() {
#ifdef WIN32
        return m_window.hwnd;
#else
        return m_window.view;
#endif
    }

	void DrawSceneEditing();

	/* use libobs allocator for alignment because the matrices itemToScreen
	* and screenToItem may contain SSE data, which will cause SSE
	* instructions to crash if the data is not aligned to at least a 16
	* byte boundry. */
	static inline void* operator new(size_t size) { return bmalloc(size); }
	static inline void operator delete(void* ptr) { bfree(ptr); }


	//是否允许选择源
	int setSourceSelectionDisabled(const char *srcName, bool disable);
	bool getSourceSelectionDisabled(const char *srcName);

	void createDisplay(void *window);
	void resetDisplay();
	inline void setScene(OBSScene scene) { m_scene = scene; }
	inline void setMouseTransferEnable(bool enable) override { m_select_enable = enable; }
	inline bool getMouseTransfeEnable() { return m_select_enable; }

	void uninitialize();
private:

    vec2 GetMouseEventPos(Broadcast::SMouseEvent *event);
    static bool DrawSelectedItem(obs_scene_t *scene, obs_sceneitem_t *item,
        void *param);

    OBSSceneItem GetItemAtPos(const vec2 &pos, bool selectBelow);
	obs_source_t * SelectedAtPos(const vec2 &pos, bool &select);
    OBSSceneItem rightSelectedAtPos(const vec2 &pos);

    void DoSelect(const vec2 &pos);
    void DoCtrlSelect(const vec2 &pos);

    vec3 GetSnapOffset(const vec3 &tl, const vec3 &br);

	void GetStretchHandleData(const vec2 &pos);

	void SnapStretchingToScreen(vec3 &tl, vec3 &br);
	void ClampAspect(vec3 &tl, vec3 &br, vec2 &size, const vec2 &baseSize);
	vec3 CalculateStretchPos(const vec3 &tl, const vec3 &br);
	void CropItem(const vec2 &pos);
	void StretchItem(const vec2 &pos);

    void SnapItemMovement(vec2 &offset);
	void MoveItems(const vec2 &pos);

	void ProcessClick(const vec2 &pos);


	static void DrawCallback(void *data, uint32_t cx, uint32_t cy);
private:

	obs_sceneitem_crop startCrop;
	vec2         startItemPos;
	vec2         cropSize;
	OBSSceneItem stretchItem;
	ItemHandle   stretchHandle = ItemHandle::None;
	vec2         stretchItemSize;
	matrix4      screenToItem;
	matrix4      itemToScreen;

	vec2         startPos;
	vec2         lastMoveOffset;
	bool         mouseDown = false;
	bool         mouseMoved = false;
	bool         mouseOverItems = false;
	bool         cropping = false;

	bool		m_select_enable = true;

	obs_display_t *m_display = nullptr;

	gs_rect m_rect = { 0 };
	gs_window m_window = { 0 };


	std::shared_ptr<SourceManager> m_sourceManager;
	OBSScene m_scene;
	std::list<std::string> m_listSelectionEnabled;
	GeometryInfo m_geometry_info;

	bool m_create_display_thread_stop = false;

	std::thread m_craete_display_tread;

	friend class SourceManager;
	friend class CCEngine;
};
