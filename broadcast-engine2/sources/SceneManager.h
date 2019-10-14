#ifndef CCScene_h__
#define CCScene_h__

#include "IBroadcastEngine.h"
#include <obs.hpp>
#include <map>
#include <list>
#include <mutex>
#include <string>

class ScenePreview;

class SceneManager : public ISceneManager
{
public:
	SceneManager();
	~SceneManager();

	typedef struct _SceneInfo
	{
		OBSScene scene;
		std::string scene_name;
	}SceneInfo;

public:
	// impl ISceneManager pure virtual method.
	int setCurrentSceneName(const char *scene_name) override;
	const char * getCurrentSceneName() override;
	int createScene(const char *scene_name) override;
	void enumScene(std::function<bool(const char *scene_name)> callback) override;
	 void enumSceneItem(const char *scene_name
		, std::function<bool(Broadcast::ESourceType type, const char *source_name) > callback) override;

	bool hasScene(const char *scene_name) override;


	IScenePreview * createPreview(void *window) override;
	void releasePreview(IScenePreview *instance) override;
	
	void setItemSelection(const char *src_name, bool select) override;
	bool getItemSelected(const char *src_name) override;


	void clearItemSelected(const char *scene_name) override;
	void activeAllItem(const char *scene_name) override;
	void deactiveAllItem(const char *scene_name) override;

	void zoomAllSceneItem(float factor) override;
public:
	void addScene(obs_scene_t *scene);
	OBSScene currentScene();
	OBSSceneItem findSceneItem(const char *source_name);
	OBSScene findScene(const char *scene_name);
private:
	const char * getSceneName(OBSScene scene);

	void addSceneInfo(std::shared_ptr<SceneInfo> scene_info);
	int removeSceneInfo(const char *scene_name);
	std::shared_ptr<SceneInfo> findSceneInfo(const char *scene_name);
	std::map<std::string, std::shared_ptr<SceneInfo>> sceneInfoMap();
	void clearSceneInfo();

	void registerSceneItemCallback(OBSScene scene);
private:
	const std::map<std::string, std::shared_ptr<SceneInfo>> m_scene_map;
	std::recursive_mutex m_scene_list_mutex;
	std::shared_ptr<SceneInfo> m_current_info;
	std::once_flag m_once_flag;

	std::list<ScenePreview*> m_preview_list;
};



#endif // CCScene_h__
