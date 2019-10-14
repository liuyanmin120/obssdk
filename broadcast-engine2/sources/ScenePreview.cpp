//#include <QGuiApplication>
//#include <QMouseEvent>

#include <algorithm>
#include <cmath>
#include <graphics/vec4.h>
#include <graphics/matrix4.h>
#include "ScenePreview.h"
//#include "window-basic-main.hpp"
//#include "obs-app.hpp"
#include "SourceManager.h"
#include "BroadcastEngineImpl.h"
#include "SceneManager.h"
#include "display-helpers.h"
#include "PublicHeader.h"

#define HANDLE_RADIUS     4.0f
//敏感区
#define HANDLE_SEL_RADIUS (HANDLE_RADIUS * 1.5f)

#ifdef WIN32
#include <windows.h>
#endif

#ifndef WIN32
#include "platform/platform.hpp"
#endif


/* TODO: make C++ math classes and clean up code here later */

ScenePreview::ScenePreview(std::shared_ptr<SourceManager> src):m_sourceManager(src)
{
    memset(&m_window,0,sizeof(struct gs_window));

	blog(LOG_INFO, "construct preview %p", this);
}


ScenePreview::~ScenePreview()
{
	if (m_craete_display_tread.joinable())
	{
		m_create_display_thread_stop = true;
		m_craete_display_tread.join();
	}

	uninitialize();
	blog(LOG_INFO, "destory preview %p", this);
}

vec2 ScenePreview::GetMouseEventPos(Broadcast::SMouseEvent *event)
{
    float pixelRatio = m_sourceManager->devicePixelRatio();
	float scale = pixelRatio / m_geometry_info.toBaseScale;
	vec2 pos;
	vec2_set(&pos,
        (float(event->x) - m_geometry_info.x / pixelRatio) * scale,
        (float(event->y) - m_geometry_info.y / pixelRatio) * scale);

	return pos;
}

struct SceneFindData {
	const vec2   &pos;
	OBSSceneItem item;
	bool         selectBelow;

	SceneFindData(const SceneFindData &) = delete;
	SceneFindData(SceneFindData &&) = delete;
	SceneFindData& operator=(const SceneFindData &) = delete;
	SceneFindData& operator=(SceneFindData &&) = delete;

	inline SceneFindData(const vec2 &pos_, bool selectBelow_)
		: pos         (pos_),
		  selectBelow (selectBelow_)
	{}
};

struct SuperSceneFindData
{
	void *ptr;
	SceneFindData *data;
};

static bool SceneItemHasVideo(obs_sceneitem_t *item)
{
	obs_source_t *source = obs_sceneitem_get_source(item);
	uint32_t flags = obs_source_get_output_flags(source);
	return (flags & OBS_SOURCE_VIDEO) != 0;
}

static bool CloseFloat(float a, float b, float epsilon=0.01)
{
	using std::abs;
	return abs(a-b) <= epsilon;
}

static bool FindItemAtPos(obs_scene_t *scene, obs_sceneitem_t *item,
		void *param)
{
//    blog(LOG_ERROR,"%s+++++%d+++++++",__FUNCTION__,__LINE__);

	SceneFindData *data = reinterpret_cast<SceneFindData*>(param);
	matrix4       transform;
	matrix4       invTransform;
	vec3          transformedPos;
	vec3          pos3;
	vec3          pos3_;

	if (!SceneItemHasVideo(item))
		return true;

	vec3_set(&pos3, data->pos.x, data->pos.y, 0.0f);

	obs_sceneitem_get_box_transform(item, &transform);

	matrix4_inv(&invTransform, &transform);
	vec3_transform(&transformedPos, &pos3, &invTransform);
	vec3_transform(&pos3_, &transformedPos, &transform);

	if (CloseFloat(pos3.x, pos3_.x) && CloseFloat(pos3.y, pos3_.y) &&
	    transformedPos.x >= 0.0f && transformedPos.x <= 1.0f &&
	    transformedPos.y >= 0.0f && transformedPos.y <= 1.0f) {
		if (data->selectBelow && obs_sceneitem_selected(item)) {
			if (data->item)
				return false;
			else
				data->selectBelow = false;
		}
//        blog(LOG_ERROR,"%s+++++%d+++++++",__FUNCTION__,__LINE__);

		data->item = item;
	}

	UNUSED_PARAMETER(scene);
	return true;
}

static vec3 GetTransformedPos(float x, float y, const matrix4 &mat)
{
	vec3 result;
	vec3_set(&result, x, y, 0.0f);
	vec3_transform(&result, &result, &mat);
	return result;
}

static vec3 GetTransformedPosScaled(float x, float y, const matrix4 &mat,
		float scale)
{
	vec3 result;
	vec3_set(&result, x, y, 0.0f);
	vec3_transform(&result, &result, &mat);
	vec3_mulf(&result, &result, scale);
	return result;
}

static inline vec2 GetOBSScreenSize()
{
	obs_video_info ovi;
	vec2 size;
	vec2_zero(&size);

	if (obs_get_video_info(&ovi)) {
		size.x = float(ovi.base_width);
		size.y = float(ovi.base_height);
	}

	return size;
}

vec3 ScenePreview::GetSnapOffset(const vec3 &tl, const vec3 &br)
{
//	OBSBasic *main = reinterpret_cast<OBSBasic*>(App()->GetMainWindow());
	vec2 screenSize = GetOBSScreenSize();
	vec3 clampOffset;

	vec3_zero(&clampOffset);

    const bool snap = config_get_bool(m_sourceManager->globalConfig(),
			"BasicWindow", "SnappingEnabled");
	if (snap == false)
		return clampOffset;

    const bool screenSnap = config_get_bool(m_sourceManager->globalConfig(),
			"BasicWindow", "ScreenSnapping");
    const bool centerSnap = config_get_bool(m_sourceManager->globalConfig(),
			"BasicWindow", "CenterSnapping");

    const float clampDist = config_get_double(m_sourceManager->globalConfig(),
			"BasicWindow", "SnapDistance") / m_geometry_info.toBaseScale;
	const float centerX = br.x - (br.x - tl.x) / 2.0f;
	const float centerY = br.y - (br.y - tl.y) / 2.0f;

	// Left screen edge.
	if (screenSnap &&
	    fabsf(tl.x) < clampDist)
		clampOffset.x = -tl.x;
	// Right screen edge.
	if (screenSnap &&
	    fabsf(clampOffset.x) < EPSILON &&
	    fabsf(screenSize.x - br.x) < clampDist)
		clampOffset.x = screenSize.x - br.x;
	// Horizontal center.
	if (centerSnap &&
	    fabsf(screenSize.x - (br.x - tl.x)) > clampDist &&
	    fabsf(screenSize.x / 2.0f - centerX) < clampDist)
		clampOffset.x = screenSize.x / 2.0f - centerX;

	// Top screen edge.
	if (screenSnap &&
	    fabsf(tl.y) < clampDist)
		clampOffset.y = -tl.y;
	// Bottom screen edge.
	if (screenSnap &&
	    fabsf(clampOffset.y) < EPSILON &&
	    fabsf(screenSize.y - br.y) < clampDist)
		clampOffset.y = screenSize.y - br.y;
	// Vertical center.
	if (centerSnap &&
	    fabsf(screenSize.y - (br.y - tl.y)) > clampDist &&
	    fabsf(screenSize.y / 2.0f - centerY) < clampDist)
		clampOffset.y = screenSize.y / 2.0f - centerY;

	return clampOffset;
}

OBSSceneItem ScenePreview::GetItemAtPos(const vec2 &pos, bool selectBelow)
{
//	OBSBasic *main = reinterpret_cast<OBSBasic*>(App()->GetMainWindow());
//    blog(LOG_ERROR,"%s+++++%d+++++++",__FUNCTION__,__LINE__);

	if (!m_scene)
		return OBSSceneItem();

	SceneFindData data(pos, selectBelow);
	obs_scene_enum_items(m_scene, FindItemAtPos, &data);
	return data.item;
}

static bool CheckItemSelected(obs_scene_t *scene, obs_sceneitem_t *item,
		void *param)
{
	SuperSceneFindData *sdata = reinterpret_cast<SuperSceneFindData*>(param);
	SceneFindData *data = (SceneFindData *)sdata->data;
	ScenePreview *preview = (ScenePreview *)sdata->ptr;

	matrix4       transform;
	vec3          transformedPos;
	vec3          pos3;

	if (!SceneItemHasVideo(item))
		return true;

	vec3_set(&pos3, data->pos.x, data->pos.y, 0.0f);

	obs_sceneitem_get_box_transform(item, &transform);

	matrix4_inv(&transform, &transform);
	vec3_transform(&transformedPos, &pos3, &transform);

	if (transformedPos.x >= 0.0f && transformedPos.x <= 1.0f &&
	    transformedPos.y >= 0.0f && transformedPos.y <= 1.0f) {
		obs_source_t *source = obs_sceneitem_get_source(item);
		const char *name = obs_source_get_name(source);
		if (name && !preview->getSourceSelectionDisabled(name))
		{
			if (obs_sceneitem_selected(item)) {
				data->item = item;
				return false;
			}
		}
		else
		{
			obs_sceneitem_select(item, false);
			return false;
		}
	}

	UNUSED_PARAMETER(scene);
	return true;
}

static bool rightMouseItemSelected(obs_scene_t *scene, obs_sceneitem_t *item,
        void *param)
{

    SceneFindData *data = reinterpret_cast<SceneFindData*>(param);
    matrix4       transform;
    vec3          transformedPos;
    vec3          pos3;

    if (!SceneItemHasVideo(item))
        return true;

    vec3_set(&pos3, data->pos.x, data->pos.y, 0.0f);

    obs_sceneitem_get_box_transform(item, &transform);

    matrix4_inv(&transform, &transform);
    vec3_transform(&transformedPos, &pos3, &transform);

    if (transformedPos.x >= 0.0f && transformedPos.x <= 1.0f &&
        transformedPos.y >= 0.0f && transformedPos.y <= 1.0f) 
	{
        if (obs_sceneitem_selected(item)) {
            data->item = item;
            return false;
        }
    }

    UNUSED_PARAMETER(scene);
    return true;
}

OBSSceneItem ScenePreview::rightSelectedAtPos(const vec2 &pos)
{
    if (!m_scene)
        return nullptr;

    SceneFindData data(pos, false);
    obs_scene_enum_items(m_scene, rightMouseItemSelected, &data);

    return data.item;

}

obs_source_t * ScenePreview::SelectedAtPos(const vec2 &pos, bool &select)
{
	if (!m_scene)
		return nullptr;

	SceneFindData data(pos, false);

	SuperSceneFindData sdata;
	sdata.data = &data;
	sdata.ptr = this;
	obs_scene_enum_items(m_scene, CheckItemSelected, &sdata);
	select = !!data.item;
	return obs_sceneitem_get_source(data.item);
}

struct HandleFindData {
	const vec2   &pos;
	const float  scale;

	OBSSceneItem item;
	ItemHandle   handle = ItemHandle::None;

	HandleFindData(const HandleFindData &) = delete;
	HandleFindData(HandleFindData &&) = delete;
	HandleFindData& operator=(const HandleFindData &) = delete;
	HandleFindData& operator=(HandleFindData &&) = delete;

	inline HandleFindData(const vec2 &pos_, float scale_)
		: pos   (pos_),
		  scale (scale_)
	{}
};

static bool FindHandleAtPos(obs_scene_t *scene, obs_sceneitem_t *item,
		void *param)
{

	if (!obs_sceneitem_selected(item))
		return true;
	HandleFindData *data = reinterpret_cast<HandleFindData*>(param);
	matrix4        transform;
	vec3           pos3;
	float          closestHandle = HANDLE_SEL_RADIUS;

	vec3_set(&pos3, data->pos.x, data->pos.y, 0.0f);

	obs_sceneitem_get_box_transform(item, &transform);

	auto TestHandle = [&] (float x, float y, ItemHandle handle)
	{
		vec3 handlePos = GetTransformedPosScaled(x, y, transform,
				data->scale);

		float dist = vec3_dist(&handlePos, &pos3);
		if (dist < HANDLE_SEL_RADIUS) {
			if (dist < closestHandle) {
				closestHandle = dist;
				data->handle  = handle;
				data->item    = item;
			}
		}
	};

	TestHandle(0.0f, 0.0f, ItemHandle::TopLeft);
	TestHandle(0.5f, 0.0f, ItemHandle::TopCenter);
	TestHandle(1.0f, 0.0f, ItemHandle::TopRight);
	TestHandle(0.0f, 0.5f, ItemHandle::CenterLeft);
	TestHandle(1.0f, 0.5f, ItemHandle::CenterRight);
	TestHandle(0.0f, 1.0f, ItemHandle::BottomLeft);
	TestHandle(0.5f, 1.0f, ItemHandle::BottomCenter);
	TestHandle(1.0f, 1.0f, ItemHandle::BottomRight);

	UNUSED_PARAMETER(scene);
	return true;
}

static vec2 GetItemSize(obs_sceneitem_t *item)
{
	obs_bounds_type boundsType = obs_sceneitem_get_bounds_type(item);
	vec2 size;

	if (boundsType != OBS_BOUNDS_NONE) {
		obs_sceneitem_get_bounds(item, &size);
	} else {
		obs_source_t *source = obs_sceneitem_get_source(item);
		obs_sceneitem_crop crop;
		vec2 scale;

		obs_sceneitem_get_scale(item, &scale);
		obs_sceneitem_get_crop(item, &crop);
		size.x = float(obs_source_get_width(source) -
				crop.left - crop.right) * scale.x;
		size.y = float(obs_source_get_height(source) -
				crop.top - crop.bottom) * scale.y;
	}

    return size;
}

void ScenePreview::GetStretchHandleData(const vec2 &pos)
{
	if (!m_scene)
		return;

    HandleFindData data(pos, m_geometry_info.toBaseScale / m_sourceManager->devicePixelRatio());
	obs_scene_enum_items(m_scene, FindHandleAtPos, &data);

	stretchItem     = std::move(data.item);
	stretchHandle   = data.handle;

	if (stretchHandle != ItemHandle::None) {
		matrix4 boxTransform;
		vec3    itemUL;
		float   itemRot;

		stretchItemSize = GetItemSize(stretchItem);

		obs_sceneitem_get_box_transform(stretchItem, &boxTransform);
		itemRot = obs_sceneitem_get_rot(stretchItem);
		vec3_from_vec4(&itemUL, &boxTransform.t);

		/* build the item space conversion matrices */
		matrix4_identity(&itemToScreen);
		matrix4_rotate_aa4f(&itemToScreen, &itemToScreen,
				0.0f, 0.0f, 1.0f, RAD(itemRot));
		matrix4_translate3f(&itemToScreen, &itemToScreen,
				itemUL.x, itemUL.y, 0.0f);

		matrix4_identity(&screenToItem);
		matrix4_translate3f(&screenToItem, &screenToItem,
				-itemUL.x, -itemUL.y, 0.0f);
		matrix4_rotate_aa4f(&screenToItem, &screenToItem,
				0.0f, 0.0f, 1.0f, RAD(-itemRot));

		obs_sceneitem_get_crop(stretchItem, &startCrop);
		obs_sceneitem_get_pos(stretchItem, &startItemPos);

		obs_source_t *source = obs_sceneitem_get_source(stretchItem);
		cropSize.x = float(obs_source_get_width(source) -
				startCrop.left - startCrop.right);
		cropSize.y = float(obs_source_get_height(source) -
				startCrop.top - startCrop.bottom);
	}
}


const char* ScenePreview::mousePressEvent(Broadcast::SMouseEvent *event)
{
	if (!m_select_enable)
		return nullptr;

	float pixelRatio = m_sourceManager->devicePixelRatio();
	float x = float(event->x) - m_geometry_info.x / pixelRatio;
	float y = float(event->y) - m_geometry_info.y / pixelRatio;
	modifiers = event->modifiers;
	bool altDown = (modifiers & Broadcast::EKey::KEY_ALT);


	if (event->button != Broadcast::EMouseButton::MOUSE_LEFT &&
		event->button != Broadcast::EMouseButton::MOUSE_RIGHT)
		return nullptr;

	if (event->button == Broadcast::EMouseButton::MOUSE_LEFT)
		mouseDown = true;

	if (altDown)
		cropping = true;

	vec2_set(&startPos, x, y);
	GetStretchHandleData(startPos);

	vec2_divf(&startPos, &startPos, m_geometry_info.toBaseScale / pixelRatio);
	startPos.x = std::round(startPos.x);
	startPos.y = std::round(startPos.y);

	obs_source_t *source = SelectedAtPos(startPos, mouseOverItems);
	vec2_zero(&lastMoveOffset);
	return obs_source_get_name(source);
}

static bool select_one(obs_scene_t *scene, obs_sceneitem_t *item, void *param)
{
	obs_sceneitem_t *selectedItem =
		reinterpret_cast<obs_sceneitem_t*>(param);
	obs_sceneitem_select(item, (selectedItem == item));

	UNUSED_PARAMETER(scene);
	return true;
}

void ScenePreview::DoSelect(const vec2 &pos)
{
	OBSSceneItem item  = GetItemAtPos(pos, false);
	if (!item)
	{
		obs_scene_enum_items(m_scene, [](obs_scene_t *scene, obs_sceneitem_t *item, void *param)
		{
			if (obs_sceneitem_selected(item))
			{
				obs_sceneitem_select(item, false);
				return false;
			}
			return true;
		}, nullptr);
		return;
	}

    obs_source_t *source = obs_sceneitem_get_source(item);

    auto src = obs_source_get_name(source);
    auto logo = m_sourceManager->getLogoSourceName();
    if(src&&logo&&strcmp(src,logo)==0){
        return;
    }
	if (getSourceSelectionDisabled(src))
	{
		return;
	}

    obs_scene_enum_items(m_scene, select_one, (obs_sceneitem_t*)item);
}

void ScenePreview::DoCtrlSelect(const vec2 &pos)
{
	OBSSceneItem item = GetItemAtPos(pos, false);
	if (!item)
		return;

    obs_source_t *source = obs_sceneitem_get_source(item);
    auto src = obs_source_get_name(source);
    auto logo = m_sourceManager->getLogoSourceName();
    if(src&&logo&&strcmp(src,logo)==0){
        return;
    }
	if (getSourceSelectionDisabled(src))
	{
		obs_sceneitem_select(item, false);
		return;
	}

	bool selected = obs_sceneitem_selected(item);
	obs_sceneitem_select(item, !selected);
}

void ScenePreview::ProcessClick(const vec2 &pos)
{
//    Qt::KeyboardModifiers modifiers = QGuiApplication::keyboardModifiers();
    //vec2 pos = GetMouseEventPos(event);

    if (modifiers & Broadcast::EKey::KEY_CONTROL)
        DoCtrlSelect(pos);
	else
		DoSelect(pos);
}

void ScenePreview::createDisplay(void *window)
{
#ifdef WIN32
	m_window.hwnd = window;
#else
    m_window.view = (__unsafe_unretained id &)window;
#endif
	

	if (m_craete_display_tread.joinable())
	{
		m_create_display_thread_stop = true;
		m_craete_display_tread.join();
	}
	m_craete_display_tread = std::thread([this]()
	{
		while (!m_create_display_thread_stop)
		{
#ifdef WIN32
			if (IsWindowVisible((HWND)m_window.hwnd))
#else
			if(isWindowHidden(m_window.view))//TODO. check window is visible.)
#endif
			{
#ifdef WIN32
				RECT rc;
				GetWindowRect((HWND)m_window.hwnd, &rc);
				m_geometry_info.window_width = rc.right - rc.left;
				m_geometry_info.window_height = rc.bottom - rc.top;
#else
				//TODO. get window size.
                std::this_thread::sleep_for(std::chrono::milliseconds(100));

                struct cs_rect rect = {0};
                rect = GetWindowRect(m_window.view);
                m_geometry_info.window_width = rect.cx * m_sourceManager->devicePixelRatio();
                m_geometry_info.window_height = rect.cy * m_sourceManager->devicePixelRatio();
#endif

				obs_video_info ovi;

				obs_get_video_info(&ovi);
                
            
				GetScaleAndCenterPos(ovi.base_width, ovi.base_height,
					m_geometry_info.window_width,
					m_geometry_info.window_height,
					m_geometry_info.x, m_geometry_info.y, m_geometry_info.toBaseScale);
                

                gs_init_data init_data = {};


				init_data.cx = m_geometry_info.window_width;
				init_data.cy = m_geometry_info.window_height;
				init_data.format = GS_BGRA;
				init_data.zsformat = GS_ZS_NONE;

				init_data.window = m_window;

				obs_display_remove_draw_callback(m_display, ScenePreview::DrawCallback, this);
				obs_display_destroy(m_display);

				m_display = obs_display_create(&init_data, GREY_COLOR_BACKGROUND);
				obs_display_add_draw_callback(m_display, ScenePreview::DrawCallback, this);
				break;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	});
}


void ScenePreview::resetDisplay()
{
#ifdef WIN32
	createDisplay(m_window.hwnd);
#else
    createDisplay(m_window.view);
#endif
}


void ScenePreview::uninitialize()
{
	obs_display_remove_draw_callback(m_display, ScenePreview::DrawCallback, this);
	obs_display_destroy(m_display);
	m_display = nullptr;
	m_scene = nullptr;
}

void ScenePreview::DrawCallback(void *data, uint32_t cx, uint32_t cy)
{

	ScenePreview *pThis = static_cast<ScenePreview*>(data);

	obs_video_info ovi;

	obs_get_video_info(&ovi);


	auto w_width = int(pThis->m_geometry_info.toBaseScale * float(ovi.base_width));
	auto w_height = int(pThis->m_geometry_info.toBaseScale * float(ovi.base_height));

	gs_viewport_push();
	gs_projection_push();

	gs_ortho(0.0f, float(ovi.base_width), 0.0f, float(ovi.base_height),
		-100.0f, 100.0f);
	gs_set_viewport(pThis->m_geometry_info.x, pThis->m_geometry_info.y,
		w_width, w_height);

	pThis->m_sourceManager->DrawBackdrop(float(ovi.base_width), float(ovi.base_height));

	auto source = obs_scene_get_source(pThis->m_scene);
	obs_source_video_render(source);
	//obs_render_main_view();
	gs_load_vertexbuffer(nullptr);

	float right = float(pThis->m_geometry_info.window_width) - pThis->m_geometry_info.x;
	float bottom = float(pThis->m_geometry_info.window_height) - pThis->m_geometry_info.y;

	gs_ortho(-pThis->m_geometry_info.x, right,
		-pThis->m_geometry_info.y, bottom,
		-100.0f, 100.0f);
	gs_reset_viewport();

	pThis->DrawSceneEditing();

	gs_projection_pop();
	gs_viewport_pop();

	UNUSED_PARAMETER(cx);
	UNUSED_PARAMETER(cy);
}

void ScenePreview::mouseReleaseEvent(Broadcast::SMouseEvent *event)
{
	if (mouseDown) {
		vec2 pos = GetMouseEventPos(event);

		if (!mouseMoved)
			ProcessClick(pos);

		stretchItem = nullptr;
		mouseDown = false;
		mouseMoved = false;
		cropping = false;
	}
}

struct SelectedItemBounds {
	bool first = true;
	vec3 tl, br;
};

static bool AddItemBounds(obs_scene_t *scene, obs_sceneitem_t *item,
		void *param)
{
	SelectedItemBounds *data = reinterpret_cast<SelectedItemBounds*>(param);

	if (!obs_sceneitem_selected(item))
		return true;

	matrix4 boxTransform;
	obs_sceneitem_get_box_transform(item, &boxTransform);

	vec3 t[4] = {
		GetTransformedPos(0.0f, 0.0f, boxTransform),
		GetTransformedPos(1.0f, 0.0f, boxTransform),
		GetTransformedPos(0.0f, 1.0f, boxTransform),
		GetTransformedPos(1.0f, 1.0f, boxTransform)
	};

	for (const vec3 &v : t) {
		if (data->first) {
			vec3_copy(&data->tl, &v);
			vec3_copy(&data->br, &v);
			data->first = false;
		} else {
			vec3_min(&data->tl, &data->tl, &v);
			vec3_max(&data->br, &data->br, &v);
		}
	}

	UNUSED_PARAMETER(scene);
	return true;
}

struct OffsetData {
	float clampDist;
	vec3 tl, br, offset;
};

static bool GetSourceSnapOffset(obs_scene_t *scene, obs_sceneitem_t *item,
		void *param)
{
	OffsetData *data = reinterpret_cast<OffsetData*>(param);

	if (obs_sceneitem_selected(item))
		return true;

	matrix4 boxTransform;
	obs_sceneitem_get_box_transform(item, &boxTransform);

	vec3 t[4] = {
		GetTransformedPos(0.0f, 0.0f, boxTransform),
		GetTransformedPos(1.0f, 0.0f, boxTransform),
		GetTransformedPos(0.0f, 1.0f, boxTransform),
		GetTransformedPos(1.0f, 1.0f, boxTransform)
	};

	bool first = true;
	vec3 tl, br;
	vec3_zero(&tl);
	vec3_zero(&br);
	for (const vec3 &v : t) {
		if (first) {
			vec3_copy(&tl, &v);
			vec3_copy(&br, &v);
			first = false;
		} else {
			vec3_min(&tl, &tl, &v);
			vec3_max(&br, &br, &v);
		}
	}

	// Snap to other source edges
#define EDGE_SNAP(l, r, x, y) \
	do { \
		double dist = fabsf(l.x - data->r.x); \
		if (dist < data->clampDist && \
		    fabsf(data->offset.x) < EPSILON && \
		    data->tl.y < br.y && \
		    data->br.y > tl.y && \
		    (fabsf(data->offset.x) > dist || data->offset.x < EPSILON)) \
			data->offset.x = l.x - data->r.x; \
	} while (false)

	EDGE_SNAP(tl, br, x, y);
	EDGE_SNAP(tl, br, y, x);
	EDGE_SNAP(br, tl, x, y);
	EDGE_SNAP(br, tl, y, x);
#undef EDGE_SNAP

	UNUSED_PARAMETER(scene);
	return true;
}

void ScenePreview::SnapItemMovement(vec2 &offset)
{
	SelectedItemBounds data;
	obs_scene_enum_items(m_scene, AddItemBounds, &data);

	data.tl.x += offset.x;
	data.tl.y += offset.y;
	data.br.x += offset.x;
	data.br.y += offset.y;

	vec3 snapOffset = GetSnapOffset(data.tl, data.br);

    const bool snap = config_get_bool(m_sourceManager->globalConfig(),
			"BasicWindow", "SnappingEnabled");
    const bool sourcesSnap = config_get_bool(m_sourceManager->globalConfig(),
			"BasicWindow", "SourceSnapping");
	if (snap == false)
		return;
	if (sourcesSnap == false) {
		offset.x += snapOffset.x;
		offset.y += snapOffset.y;
		return;
	}

    const float clampDist = config_get_double(m_sourceManager->globalConfig(),
			"BasicWindow", "SnapDistance") / m_geometry_info.toBaseScale;

	OffsetData offsetData;
	offsetData.clampDist = clampDist;
	offsetData.tl = data.tl;
	offsetData.br = data.br;
	vec3_copy(&offsetData.offset, &snapOffset);

	obs_scene_enum_items(m_scene, GetSourceSnapOffset, &offsetData);

	if (fabsf(offsetData.offset.x) > EPSILON ||
	    fabsf(offsetData.offset.y) > EPSILON) {
		offset.x += offsetData.offset.x;
		offset.y += offsetData.offset.y;
	} else {
		offset.x += snapOffset.x;
		offset.y += snapOffset.y;
	}
}

static bool move_items(obs_scene_t *scene, obs_sceneitem_t *item, void *param)
{
	vec2 *offset = reinterpret_cast<vec2*>(param);

	if (obs_sceneitem_selected(item)) {
		vec2 pos;
		obs_sceneitem_get_pos(item, &pos);
		vec2_add(&pos, &pos, offset);
		obs_sceneitem_set_pos(item, &pos);
	}

	UNUSED_PARAMETER(scene);
	return true;
}

void ScenePreview::MoveItems(const vec2 &pos)
{
	vec2 offset, moveOffset;
	vec2_sub(&offset, &pos, &startPos);
	vec2_sub(&moveOffset, &offset, &lastMoveOffset);

    if (!(modifiers & Broadcast::EKey::KEY_CONTROL))
		SnapItemMovement(moveOffset);

	vec2_add(&lastMoveOffset, &lastMoveOffset, &moveOffset);

	obs_scene_enum_items(m_scene, move_items, &moveOffset);
}

vec3 ScenePreview::CalculateStretchPos(const vec3 &tl, const vec3 &br)
{
	uint32_t alignment = obs_sceneitem_get_alignment(stretchItem);
	vec3 pos;

	vec3_zero(&pos);

	if (alignment & OBS_ALIGN_LEFT)
		pos.x = tl.x;
	else if (alignment & OBS_ALIGN_RIGHT)
		pos.x = br.x;
	else
		pos.x = (br.x - tl.x) * 0.5f + tl.x;

	if (alignment & OBS_ALIGN_TOP)
		pos.y = tl.y;
	else if (alignment & OBS_ALIGN_BOTTOM)
		pos.y = br.y;
	else
		pos.y = (br.y - tl.y) * 0.5f + tl.y;

	return pos;
}

void ScenePreview::ClampAspect(vec3 &tl, vec3 &br, vec2 &size,
		const vec2 &baseSize)
{
	float    baseAspect   = baseSize.x / baseSize.y;
	float    aspect       = size.x / size.y;
	uint32_t stretchFlags = (uint32_t)stretchHandle;

	if (stretchHandle == ItemHandle::TopLeft    ||
	    stretchHandle == ItemHandle::TopRight   ||
	    stretchHandle == ItemHandle::BottomLeft ||
	    stretchHandle == ItemHandle::BottomRight) {
		if (aspect < baseAspect)
			size.x = size.y * baseAspect;
		else
			size.y = size.x / baseAspect;

	} else if (stretchHandle == ItemHandle::TopCenter ||
	           stretchHandle == ItemHandle::BottomCenter) {
		size.x = size.y * baseAspect;

	} else if (stretchHandle == ItemHandle::CenterLeft ||
	           stretchHandle == ItemHandle::CenterRight) {
		size.y = size.x / baseAspect;
	}

	size.x = std::round(size.x);
	size.y = std::round(size.y);

	if (stretchFlags & ITEM_LEFT)
		tl.x = br.x - size.x;
	else if (stretchFlags & ITEM_RIGHT)
		br.x = tl.x + size.x;

	if (stretchFlags & ITEM_TOP)
		tl.y = br.y - size.y;
	else if (stretchFlags & ITEM_BOTTOM)
		br.y = tl.y + size.y;
}

void ScenePreview::SnapStretchingToScreen(vec3 &tl, vec3 &br)
{
	uint32_t stretchFlags = (uint32_t)stretchHandle;
	vec3     newTL        = GetTransformedPos(tl.x, tl.y, itemToScreen);
	vec3     newTR        = GetTransformedPos(br.x, tl.y, itemToScreen);
	vec3     newBL        = GetTransformedPos(tl.x, br.y, itemToScreen);
	vec3     newBR        = GetTransformedPos(br.x, br.y, itemToScreen);
	vec3     boundingTL;
	vec3     boundingBR;

	vec3_copy(&boundingTL, &newTL);
	vec3_min(&boundingTL, &boundingTL, &newTR);
	vec3_min(&boundingTL, &boundingTL, &newBL);
	vec3_min(&boundingTL, &boundingTL, &newBR);

	vec3_copy(&boundingBR, &newTL);
	vec3_max(&boundingBR, &boundingBR, &newTR);
	vec3_max(&boundingBR, &boundingBR, &newBL);
	vec3_max(&boundingBR, &boundingBR, &newBR);

	vec3 offset = GetSnapOffset(boundingTL, boundingBR);
	vec3_add(&offset, &offset, &newTL);
	vec3_transform(&offset, &offset, &screenToItem);
	vec3_sub(&offset, &offset, &tl);

	if (stretchFlags & ITEM_LEFT)
		tl.x += offset.x;
	else if (stretchFlags & ITEM_RIGHT)
		br.x += offset.x;

	if (stretchFlags & ITEM_TOP)
		tl.y += offset.y;
	else if (stretchFlags & ITEM_BOTTOM)
		br.y += offset.y;
}

static float maxfunc(float x, float y)
{
	return x > y ? x : y;
}

static float minfunc(float x, float y)
{
	return x < y ? x : y;
}

void ScenePreview::CropItem(const vec2 &pos)
{
	obs_bounds_type boundsType = obs_sceneitem_get_bounds_type(stretchItem);
	uint32_t stretchFlags = (uint32_t)stretchHandle;
	uint32_t align = obs_sceneitem_get_alignment(stretchItem);
	vec3 tl, br, pos3;

	if (boundsType != OBS_BOUNDS_NONE) /* TODO */
		return;

	vec3_zero(&tl);
	vec3_set(&br, stretchItemSize.x, stretchItemSize.y, 0.0f);

	vec3_set(&pos3, pos.x, pos.y, 0.0f);
	vec3_transform(&pos3, &pos3, &screenToItem);

	obs_sceneitem_crop crop = startCrop;
	vec2 scale;

	obs_sceneitem_get_scale(stretchItem, &scale);

	vec2 max_tl;
	vec2 max_br;

	vec2_set(&max_tl,
		float(-crop.left) * scale.x,
		float(-crop.top) * scale.y);
	vec2_set(&max_br,
		stretchItemSize.x + crop.right * scale.x,
		stretchItemSize.y + crop.bottom * scale.y);

	typedef std::function<float (float, float)> minmax_func_t;

	minmax_func_t min_x = scale.x < 0.0f ? maxfunc : minfunc;
	minmax_func_t min_y = scale.y < 0.0f ? maxfunc : minfunc;
	minmax_func_t max_x = scale.x < 0.0f ? minfunc : maxfunc;
	minmax_func_t max_y = scale.y < 0.0f ? minfunc : maxfunc;

	pos3.x = min_x(pos3.x, max_br.x);
	pos3.x = max_x(pos3.x, max_tl.x);
	pos3.y = min_y(pos3.y, max_br.y);
	pos3.y = max_y(pos3.y, max_tl.y);

	if (stretchFlags & ITEM_LEFT) {
		float maxX = stretchItemSize.x - (2.0 * scale.x);
		pos3.x = tl.x = min_x(pos3.x, maxX);

	} else if (stretchFlags & ITEM_RIGHT) {
		float minX = (2.0 * scale.x);
		pos3.x = br.x = max_x(pos3.x, minX);
	}

	if (stretchFlags & ITEM_TOP) {
		float maxY = stretchItemSize.y - (2.0 * scale.y);
		pos3.y = tl.y = min_y(pos3.y, maxY);

	} else if (stretchFlags & ITEM_BOTTOM) {
		float minY = (2.0 * scale.y);
		pos3.y = br.y = max_y(pos3.y, minY);
	}

#define ALIGN_X (ITEM_LEFT|ITEM_RIGHT)
#define ALIGN_Y (ITEM_TOP|ITEM_BOTTOM)
	vec3 newPos;
	vec3_zero(&newPos);

	uint32_t align_x = (align & ALIGN_X);
	uint32_t align_y = (align & ALIGN_Y);
	if (align_x == (stretchFlags & ALIGN_X) && align_x != 0)
		newPos.x = pos3.x;
	else if (align & ITEM_RIGHT)
		newPos.x = stretchItemSize.x;
	else if (!(align & ITEM_LEFT))
		newPos.x = stretchItemSize.x * 0.5f;

	if (align_y == (stretchFlags & ALIGN_Y) && align_y != 0)
		newPos.y = pos3.y;
	else if (align & ITEM_BOTTOM)
		newPos.y = stretchItemSize.y;
	else if (!(align & ITEM_TOP))
		newPos.y = stretchItemSize.y * 0.5f;
#undef ALIGN_X
#undef ALIGN_Y

	crop = startCrop;

	if (stretchFlags & ITEM_LEFT)
		crop.left += int(std::round(tl.x / scale.x));
	else if (stretchFlags & ITEM_RIGHT)
		crop.right += int(std::round((stretchItemSize.x - br.x) / scale.x));

	if (stretchFlags & ITEM_TOP)
		crop.top += int(std::round(tl.y / scale.y));
	else if (stretchFlags & ITEM_BOTTOM)
		crop.bottom += int(std::round((stretchItemSize.y - br.y) / scale.y));

	vec3_transform(&newPos, &newPos, &itemToScreen);
	newPos.x = std::round(newPos.x);
	newPos.y = std::round(newPos.y);

#if 0
	vec3 curPos;
	vec3_zero(&curPos);
	obs_sceneitem_get_pos(stretchItem, (vec2*)&curPos);
	blog(LOG_DEBUG, "curPos {%d, %d} - newPos {%d, %d}",
			int(curPos.x), int(curPos.y),
			int(newPos.x), int(newPos.y));
	blog(LOG_DEBUG, "crop {%d, %d, %d, %d}",
			crop.left, crop.top,
			crop.right, crop.bottom);
#endif

	obs_sceneitem_defer_update_begin(stretchItem);
	obs_sceneitem_set_crop(stretchItem, &crop);
	obs_sceneitem_set_pos(stretchItem, (vec2*)&newPos);
	obs_sceneitem_defer_update_end(stretchItem);
}

void ScenePreview::StretchItem(const vec2 &pos)
{
    //vec2 pos = GetMouseEventPos(event);

//	Qt::KeyboardModifiers modifiers = QGuiApplication::keyboardModifiers();
	obs_bounds_type boundsType = obs_sceneitem_get_bounds_type(stretchItem);
	uint32_t stretchFlags = (uint32_t)stretchHandle;
    bool shiftDown = (modifiers & Broadcast::EKey::KEY_SHIFT);
	vec3 tl, br, pos3;

	vec3_zero(&tl);
	vec3_set(&br, stretchItemSize.x, stretchItemSize.y, 0.0f);

	vec3_set(&pos3, pos.x, pos.y, 0.0f);
	vec3_transform(&pos3, &pos3, &screenToItem);

	if (stretchFlags & ITEM_LEFT)
		tl.x = pos3.x;
	else if (stretchFlags & ITEM_RIGHT)
		br.x = pos3.x;

	if (stretchFlags & ITEM_TOP)
		tl.y = pos3.y;
	else if (stretchFlags & ITEM_BOTTOM)
		br.y = pos3.y;

    if (!(modifiers & Broadcast::EKey::KEY_CONTROL))
		SnapStretchingToScreen(tl, br);

	obs_source_t *source = obs_sceneitem_get_source(stretchItem);

	vec2 baseSize;
	vec2_set(&baseSize,
		float(obs_source_get_width(source)),
		float(obs_source_get_height(source)));

	vec2 size;
	vec2_set(&size,br. x - tl.x, br.y - tl.y);

	if (boundsType != OBS_BOUNDS_NONE) {
		if (shiftDown)
			ClampAspect(tl, br, size, baseSize);

		if (tl.x > br.x) std::swap(tl.x, br.x);
		if (tl.y > br.y) std::swap(tl.y, br.y);

		vec2_abs(&size, &size);

		obs_sceneitem_set_bounds(stretchItem, &size);
	} else {
		obs_sceneitem_crop crop;
		obs_sceneitem_get_crop(stretchItem, &crop);

		baseSize.x -= float(crop.left + crop.right);
		baseSize.y -= float(crop.top + crop.bottom);

		if (!shiftDown)
			ClampAspect(tl, br, size, baseSize);

		vec2_div(&size, &size, &baseSize);
		obs_sceneitem_set_scale(stretchItem, &size);
	}

	pos3 = CalculateStretchPos(tl, br);
	vec3_transform(&pos3, &pos3, &itemToScreen);

	vec2 newPos;
	vec2_set(&newPos, std::round(pos3.x), std::round(pos3.y));
	obs_sceneitem_set_pos(stretchItem, &newPos);
}

void ScenePreview::mouseMoveEvent(Broadcast::SMouseEvent *event)
{
	if (mouseDown) {
		vec2 pos = GetMouseEventPos(event);

		if (!mouseMoved && !mouseOverItems &&
			stretchHandle == ItemHandle::None) {
			ProcessClick(startPos);
			SelectedAtPos(startPos, mouseOverItems);
		}

		pos.x = std::round(pos.x);
		pos.y = std::round(pos.y);

		if (stretchHandle != ItemHandle::None) {
			if (cropping)
				CropItem(pos);
			else
				StretchItem(pos);

		}
		else if (mouseOverItems) {
			MoveItems(pos);
		}

		mouseMoved = true;
	}
}


void ScenePreview::update()
{
#ifdef WIN32
	createDisplay(m_window.hwnd);
#else
    createDisplay(m_window.view);
#endif
}

void ScenePreview::bindScene(const char * scene_name)
{
	auto scenceManger = std::dynamic_pointer_cast<SceneManager>(g_pBroadcastEngine->sceneManager());
	if (scenceManger) 
	{
		m_scene = scenceManger->findScene(scene_name);
	}
	else 
	{
		m_scene = nullptr;
	}
	
}

static void DrawCircleAtPos(float x, float y, matrix4 &matrix,
		float previewScale)
{
//    blog(LOG_ERROR,"%s:%f,%f,%f",__FUNCTION__,x,y,previewScale);

	struct vec3 pos;
	vec3_set(&pos, x, y, 0.0f);
	vec3_transform(&pos, &pos, &matrix);
	vec3_mulf(&pos, &pos, previewScale);

	gs_matrix_push();
	gs_matrix_translate(&pos);
	gs_matrix_scale3f(HANDLE_RADIUS, HANDLE_RADIUS, 1.0f);
	gs_draw(GS_LINESTRIP, 0, 0);
	gs_matrix_pop();
}

static inline bool crop_enabled(const obs_sceneitem_crop *crop)
{
	return crop->left > 0  ||
	       crop->top > 0   ||
	       crop->right > 0 ||
	       crop->bottom > 0;
}
bool ScenePreview::DrawSelectedItem(obs_scene_t *scene,
		obs_sceneitem_t *item, void *param)
{
	if (!obs_sceneitem_selected(item))
		return true;

    ScenePreview *pThis = reinterpret_cast<ScenePreview*>(param);
	
	matrix4 boxTransform;
	matrix4 invBoxTransform;
	obs_sceneitem_get_box_transform(item, &boxTransform);
	matrix4_inv(&invBoxTransform, &boxTransform);

	vec3 bounds[] = {
		{{{0.f, 0.f, 0.f}}},
		{{{1.f, 0.f, 0.f}}},
		{{{0.f, 1.f, 0.f}}},
		{{{1.f, 1.f, 0.f}}},
	};

	bool visible = std::all_of(std::begin(bounds), std::end(bounds),
			[&](const vec3 &b)
	{
		vec3 pos;
		vec3_transform(&pos, &b, &boxTransform);
		vec3_transform(&pos, &pos, &invBoxTransform);
		return CloseFloat(pos.x, b.x) && CloseFloat(pos.y, b.y);
	});

	if (!visible)
		return true;

	obs_transform_info info;
	obs_sceneitem_get_info(item, &info);

	gs_load_vertexbuffer(pThis->m_sourceManager->circle);

	DrawCircleAtPos(0.0f, 0.0f, boxTransform, pThis->m_geometry_info.toBaseScale);
	DrawCircleAtPos(0.0f, 1.0f, boxTransform, pThis->m_geometry_info.toBaseScale);
	DrawCircleAtPos(1.0f, 0.0f, boxTransform, pThis->m_geometry_info.toBaseScale);
	DrawCircleAtPos(1.0f, 1.0f, boxTransform, pThis->m_geometry_info.toBaseScale);
	DrawCircleAtPos(0.5f, 0.0f, boxTransform, pThis->m_geometry_info.toBaseScale);
	DrawCircleAtPos(0.0f, 0.5f, boxTransform, pThis->m_geometry_info.toBaseScale);
	DrawCircleAtPos(0.5f, 1.0f, boxTransform, pThis->m_geometry_info.toBaseScale);
	DrawCircleAtPos(1.0f, 0.5f, boxTransform, pThis->m_geometry_info.toBaseScale);

	gs_matrix_push();
	gs_matrix_scale3f(pThis->m_geometry_info.toBaseScale
		, pThis->m_geometry_info.toBaseScale
		, 1.0f);
	gs_matrix_mul(&boxTransform);

	obs_sceneitem_crop crop;
	obs_sceneitem_get_crop(item, &crop);

	if (info.bounds_type == OBS_BOUNDS_NONE && crop_enabled(&crop)) {
		vec4 color;
		gs_effect_t *eff = gs_get_effect();
		gs_eparam_t *param = gs_effect_get_param_by_name(eff, "color");

#define DRAW_SIDE(side, vb) \
		if (crop.side > 0) \
			vec4_set(&color, 0.0f, 1.0f, 0.0f, 1.0f); \
		else \
			vec4_set(&color, 1.0f, 0.0f, 0.0f, 1.0f); \
		gs_effect_set_vec4(param, &color); \
		gs_load_vertexbuffer(pThis->m_sourceManager->vb); \
		gs_draw(GS_LINESTRIP, 0, 0);

		DRAW_SIDE(left,   boxLeft);
		DRAW_SIDE(top,    boxTop);
		DRAW_SIDE(right,  boxRight);
		DRAW_SIDE(bottom, boxBottom);
#undef DRAW_SIDE
	} else {
		gs_load_vertexbuffer(pThis->m_sourceManager->box);
		gs_draw(GS_LINESTRIP, 0, 0);
	}

	gs_matrix_pop();

	UNUSED_PARAMETER(scene);
	UNUSED_PARAMETER(param);
	return true;
}

void ScenePreview::DrawSceneEditing()
{
//	OBSBasic *main = reinterpret_cast<OBSBasic*>(App()->GetMainWindow());

	gs_effect_t    *solid = obs_get_base_effect(OBS_EFFECT_SOLID);
	gs_technique_t *tech  = gs_effect_get_technique(solid, "Solid");

	vec4 color;
	vec4_set(&color, 1.0f, 0.0f, 0.0f, 1.0f);
	gs_effect_set_vec4(gs_effect_get_param_by_name(solid, "color"), &color);

	gs_technique_begin(tech);
	gs_technique_begin_pass(tech, 0);

	if (m_scene)
		obs_scene_enum_items(m_scene, DrawSelectedItem, this);

	gs_load_vertexbuffer(nullptr);

	gs_technique_end_pass(tech);
	gs_technique_end(tech);
}

int ScenePreview::setSourceSelectionDisabled(const char *srcName, bool disable)
{
	if (disable)
	{
		m_listSelectionEnabled.push_back(std::string(srcName));
	}
	else
	{
		for (std::list<std::string>::iterator it = m_listSelectionEnabled.begin();
			it != m_listSelectionEnabled.end();
			++it)
		{
			if ((*it).compare(srcName) == 0)
			{
				m_listSelectionEnabled.erase(it);
				break;
			}
		}
	}
	return 0;
}

bool ScenePreview::getSourceSelectionDisabled(const char *srcName)
{
	for (std::list<std::string>::iterator it = m_listSelectionEnabled.begin();
		it != m_listSelectionEnabled.end();
		++it)
	{
		if ((*it).compare(srcName) == 0)
		{
			return true;
		}
	}
	return false;
}
