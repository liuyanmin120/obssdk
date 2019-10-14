#include "SceneManager.h"
#include "ScenePreview.h"
#include "BroadcastEngineImpl.h"
//#include "AudioDispatcher.h"

#include <future>


#define FUNC_ENTER blog(LOG_INFO,"enter func : %s", __FUNCTION__)
#define FUNC_LEAVE blog(LOG_INFO,"leave func : %s", __FUNCTION__)

SceneManager::SceneManager()
{
	FUNC_ENTER;
}


SceneManager::~SceneManager()
{
	FUNC_ENTER;

	for (auto preview : m_preview_list)
	{
		delete preview;
	}
	m_preview_list.clear();
	m_current_info = nullptr;
	clearSceneInfo();
}

int SceneManager::setCurrentSceneName(const char *scene_name)
{
	FUNC_ENTER;
	
	std::string prev_scene_name;
	if (m_current_info)
	{
		if (m_current_info->scene_name.compare(scene_name) != 0)
		{
			obs_scene_enum_items(m_current_info->scene, [](obs_scene_t *scene, obs_sceneitem_t *item, void *param)
			{
				OBSSource source = obs_sceneitem_get_source(item);
				obs_source_dec_showing(source);
				auto id = obs_source_get_id(source);
				if (strcmp("dshow_input",id) == 0
					|| strcmp("av_capture_input", id) == 0)
				{
					//wait for camera deactivate
					//std::this_thread::sleep_for(std::chrono::milliseconds(500));
					//obs_wait_source_action(source,false);
				}

				//obs_source_hide_force(source);
				return true;
			}, nullptr);
		}
		else
		{
			return -1;
		}
		prev_scene_name = m_current_info->scene_name;
	}
	else 
	{
		obs_enum_sources([](void *param, obs_source_t *source)
		{
			auto name = obs_source_get_name(source);
			obs_source_hide_force(source);
			return true;
		}, this);
	}

	if (auto info = findSceneInfo(scene_name))
	{
		auto source = obs_scene_get_source(info->scene);
		auto name = obs_source_get_name(source);
		if (strcmp(name, scene_name) == 0)
		{
			m_current_info = info;
			obs_set_output_source(0, source);
		}

		//CCScene will be create new source.
		if (strcmp(scene_name, DEFAULT_SCENE) != 0 )
		{
			obs_scene_enum_items(info->scene, [](obs_scene_t *scene, obs_sceneitem_t *item, void *param)
			{
				OBSSource source = obs_sceneitem_get_source(item);
				obs_source_inc_showing(source);
				//obs_source_show_force(source);
				return true;
			}, nullptr);
		}

		std::async([=]()
		{
			g_pBroadcastEngine->engineObserver()->OnCurrentSceneChange(scene_name, prev_scene_name.c_str());
		});
		return 0;
	}
	return -2;
}

const char * SceneManager::getCurrentSceneName()
{
	FUNC_ENTER;
	if (m_current_info)
	{
		return m_current_info->scene_name.c_str();
	}
	return nullptr;
}

int SceneManager::createScene(const char *scene_name)
{
	FUNC_ENTER;
	if (auto info = findSceneInfo(scene_name))
	{
		return -1;
	}

	auto info = std::make_shared<SceneInfo>();
	info->scene = obs_scene_create(scene_name);
	info->scene_name = scene_name;

	addSceneInfo(info);

	registerSceneItemCallback(info->scene);

	obs_scene_release(info->scene);


	std::call_once(m_once_flag, []()
	{
		std::thread([]()
		{
			g_pBroadcastEngine->engineObserver()->OnSceneInitialled();
		}).detach();
	});
	return 0;
}

void SceneManager::enumScene(std::function<bool(const char *scene_name)> callback)
{
	FUNC_ENTER;
	std::lock_guard<std::recursive_mutex> locker(m_scene_list_mutex);

	for (auto it : m_scene_map)
	{
		if (!callback(it.first.c_str()))
		{
			break;
		}
	}
}

void SceneManager::enumSceneItem(const char *scene_name, std::function<bool(Broadcast::ESourceType type, const char *source_name) > callback)
{
	if (auto info = findSceneInfo(scene_name))
	{		
		std::list<std::tuple<std::string, std::string>> list;

		obs_scene_enum_items(info->scene, [](obs_scene_t *scene, obs_sceneitem_t *item, void *param)
		{
			auto list = reinterpret_cast<std::list<std::tuple<std::string, std::string>>*>(param);
			
			auto source = obs_sceneitem_get_source(item);
			auto name = obs_source_get_name(source);
			auto id = obs_source_get_id(source);

			auto tp = std::make_tuple < std::string, std::string>(id, name);
			list->push_back(tp);
			return true;
		}, &list);

		for (auto tp : list)
		{
			auto type = static_cast<Broadcast::ESourceType>(g_pBroadcastEngine->lookupByName(std::get<0>(tp).c_str()));
			if (!callback(type, std::get<1>(tp).c_str()))
			{
				break;
			}
		}
	}	
}

bool SceneManager::hasScene(const char *scene_name)
{
	auto info = findSceneInfo(scene_name);
	bool exist = !!info;

	blog(LOG_INFO, "has scene %s,%s", scene_name, exist ? "true" : "false");

	return exist;
}

IScenePreview * SceneManager::createPreview(void *window)
{
	FUNC_ENTER;
	for (auto it = m_preview_list.begin(); it != m_preview_list.end();++it)
	{
		if ((*it)->renderWindow() == window)
		{
			m_preview_list.erase(it);
			break;
		}
	}

	auto preview = new ScenePreview(g_pBroadcastEngine->sourceManger());
	preview->setRenderWindow(window);
	m_preview_list.push_back(preview);
	return preview;
}

void SceneManager::releasePreview(IScenePreview *instance)
{
	if (auto ptr = dynamic_cast<ScenePreview*>(instance))
	{
		for (auto preview : m_preview_list)
		{
			if (preview == ptr)
			{
				m_preview_list.remove(ptr);
				break;
			}
		}
		delete ptr;
	}
}

void SceneManager::setItemSelection(const char *src_name, bool select)
{
	auto list = sceneInfoMap();
	for (auto pair : list)
	{
		auto info = pair.second;
		OBSSceneItem item = obs_scene_find_source(info->scene, src_name);
		if (item)
		{
			obs_sceneitem_select(item, select);
			break;
		}
	}
}

bool SceneManager::getItemSelected(const char *src_name)
{
	auto list = sceneInfoMap();
	for (auto pair : list)
	{
		auto info = pair.second;
		OBSSceneItem item = obs_scene_find_source(info->scene, src_name);
		if (item)
		{
			return obs_sceneitem_selected(item);
		}
	}
	return false;
}

void SceneManager::clearItemSelected(const char *scene_name)
{
	auto info = findSceneInfo(scene_name);
	obs_scene_enum_items(info->scene, [](obs_scene_t *scene, obs_sceneitem_t *item, void *param)
	{
		if (obs_sceneitem_selected(item))
		{
			obs_sceneitem_select(item, false);
		}
		return  true;
	}, nullptr);
}

void SceneManager::activeAllItem(const char *scene_name)
{
	if (auto info = findSceneInfo(scene_name))
	{
		obs_scene_enum_items(info->scene, [](obs_scene_t *scene, obs_sceneitem_t *item, void *param)
		{
			auto source = obs_sceneitem_get_source(item);
			obs_source_inc_showing(source);
			//obs_source_show_force(source);
			return true;
		}, nullptr);
	}
}

void SceneManager::deactiveAllItem(const char *scene_name)
{
	if (auto info = findSceneInfo(scene_name))
	{
		obs_scene_enum_items(info->scene, [](obs_scene_t *scene, obs_sceneitem_t *item, void *param)
		{
			auto source = obs_sceneitem_get_source(item);
			obs_source_dec_showing(source);
			//obs_source_hide_force(source);
			return true;
		}, nullptr);
	}
}

void SceneManager::zoomAllSceneItem(float factor)
{
	auto list = sceneInfoMap();
	for (auto pair : list)
	{
		auto info = pair.second;
		obs_scene_enum_items(info->scene, [](obs_scene_t *scene, obs_sceneitem_t *item, void *param)
		{
			float factor = *reinterpret_cast<float*>(param);

			obs_transform_info info;
			obs_sceneitem_get_info(item, &info);

			info.pos.x = info.pos.x * factor;
			info.pos.y = info.pos.y * factor;
			info.bounds.x = info.bounds.x * factor;
			info.bounds.y = info.bounds.y * factor;
			info.scale.x = info.scale.x * factor;
			info.scale.y = info.scale.y * factor;

			obs_sceneitem_set_info(item, &info);
			return true;
		}, &factor);
	}
}

void SceneManager::addScene(obs_scene_t *scene)
{
	FUNC_ENTER;
	auto source = obs_scene_get_source(scene);
	auto name = obs_source_get_name(source);
	auto info = std::make_shared<SceneInfo>();
	info->scene = scene;
	info->scene_name = name;

	addSceneInfo(info);
	registerSceneItemCallback(info->scene);

	std::call_once(m_once_flag, []()
	{
		std::async([]()
		{
			g_pBroadcastEngine->engineObserver()->OnSceneInitialled();
		});
	});
}

OBSScene SceneManager::currentScene()
{
	FUNC_ENTER;
	if (m_current_info)
	{
		return m_current_info->scene;
	}
	return nullptr;
}

OBSSceneItem SceneManager::findSceneItem( const char *source_name)
{
	if (!source_name || strlen(source_name) == 0)
	{
		return nullptr;
	}
	auto list = sceneInfoMap();
	for (auto pair : list)
	{
		auto info = pair.second;
		if (auto item = obs_scene_find_source(info->scene, source_name))
		{
			return item;
		}
	}
	return nullptr;
}

OBSScene SceneManager::findScene(const char *scene_name)
{
	if (auto info = findSceneInfo(scene_name))
	{
		return info->scene;
	}
	return nullptr;
}

const char * SceneManager::getSceneName(OBSScene scene)
{
	FUNC_ENTER;
	auto source = obs_scene_get_source(scene);
	return obs_source_get_name(source);
}

void SceneManager::addSceneInfo(std::shared_ptr<SceneInfo> scene_info)
{
	FUNC_ENTER;
	std::lock_guard<std::recursive_mutex> locker(m_scene_list_mutex);
	auto list = const_cast<std::map<std::string, std::shared_ptr<SceneInfo>>*>(&m_scene_map);
	(*list)[scene_info->scene_name] = scene_info;
}

int SceneManager::removeSceneInfo(const char *scene_name)
{
	FUNC_ENTER;
	if (!scene_name)
	{
		return -2;
	}
	std::lock_guard<std::recursive_mutex> locker(m_scene_list_mutex);
	auto list = const_cast<std::map<std::string, std::shared_ptr<SceneInfo>>*>(&m_scene_map);
	auto it = list->find(scene_name);
	if (it != list->end())
	{
		list->erase(it);
		return 0;
	}
	return -1;
}

std::shared_ptr<SceneManager::SceneInfo> SceneManager::findSceneInfo(const char *scene_name)
{
	FUNC_ENTER;
	do 
	{
		if (!scene_name)
			break;
		std::lock_guard<std::recursive_mutex> locker(m_scene_list_mutex);
		auto it = m_scene_map.find(scene_name);
		if (it != m_scene_map.end())
		{
			return it->second;
		}
	} while (0);

	return nullptr;
}

std::map<std::string, std::shared_ptr<SceneManager::SceneInfo>> SceneManager::sceneInfoMap()
{
	std::lock_guard<std::recursive_mutex> locker(m_scene_list_mutex);
	return m_scene_map;
}

void SceneManager::clearSceneInfo()
{
	FUNC_ENTER;
	std::lock_guard<std::recursive_mutex> locker(m_scene_list_mutex);
	auto list = const_cast<std::map<std::string, std::shared_ptr<SceneInfo>>*>(&m_scene_map);
	list->clear();
}

void SceneManager::registerSceneItemCallback(OBSScene scene)
{
	auto source = obs_scene_get_source(scene);
	auto handle = obs_source_get_signal_handler(source);
	
	signal_handler_connect(obs_source_get_signal_handler(source), "item_select",
		[](void *param, calldata_t *data)
	{

		auto pThis = reinterpret_cast<SceneManager*>(param);

		auto item = (obs_sceneitem_t*)calldata_ptr(data, "item");
		if (OBSSource source = obs_sceneitem_get_source(item))
		{
			std::string name = obs_source_get_name(source);

			std::async([name]()
			{
				g_pBroadcastEngine->engineObserver()->OnSourceSelectionChange(name.c_str(), true);
			});
		}
		
	}, this);

	signal_handler_connect(obs_source_get_signal_handler(source), "item_deselect",
		[](void *param, calldata_t *data)
	{

		auto pThis = reinterpret_cast<SceneManager*>(param);

		auto item = (obs_sceneitem_t*)calldata_ptr(data, "item");
		if (OBSSource source = obs_sceneitem_get_source(item))
		{
			std::string  name = obs_source_get_name(source);
			std::async([name]()
			{
				g_pBroadcastEngine->engineObserver()->OnSourceSelectionChange(name.c_str(), false);
			});
		}
	}, this);

#if 0
	signal_handler_connect(obs_source_get_signal_handler(source), "item_add",
		[](void *param, calldata_t *data)
	{
		auto pThis = reinterpret_cast<SceneManager*>(param);

		auto item = (obs_sceneitem_t*)calldata_ptr(data, "item");
		if (OBSSource source = obs_sceneitem_get_source(item))
		{
			std::string  name = obs_source_get_name(source);
			std::string id = obs_source_get_id(source);
			if (id.compare("speech_recognition") == 0 && g_pCCEngine)
			{
				if (auto audio_dispatch = g_pCCEngine->audioDispatcher())
				{
					audio_dispatch->addDispatchSource(source);
				}
			}
		}
	}, this);

	signal_handler_connect(obs_source_get_signal_handler(source), "item_remove",
		[](void *param, calldata_t *data)
	{

		auto pThis = reinterpret_cast<SceneManager*>(param);

		auto item = (obs_sceneitem_t*)calldata_ptr(data, "item");
		
		if (OBSSource source = obs_sceneitem_get_source(item))
		{
			std::string  name = obs_source_get_name(source);
			std::string id = obs_source_get_id(source);
			if (id.compare("speech_recognition") == 0)
			{
				if (auto audio_dispatch = g_pCCEngine->audioDispatcher())
				{
					audio_dispatch->removeDispatchSource(source);
				}
			}
		}
	}, this);
#endif
}
