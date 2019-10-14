#pragma once
#include <fstream>
#include <unordered_map>
#include "IBroadcastEngine.h"
#include "obs.hpp"
#include "window-basic-main-outputs.hpp"
#include "util/util.hpp"


struct AddSourceData
{
	obs_source_t *source;
	bool visible;

};

class SourceManager;
class SceneManager;
//struct BasicOutputHandler;
class BroadcastEngineImpl :public IBroadcastEngine
{
public:
	BroadcastEngineImpl(IBroadcastEngineObserver *);
	virtual ~BroadcastEngineImpl();
public:
	static void do_log(int log_level, const char *msg, va_list args, void *param);
	void  logSink(const std::string &msg, int log_level);

public:
//IBroadcastEngine impl
//初始化SDK
	virtual int init()override;
	// 加载/保存 数据源文件  
	virtual int load() override;
	virtual int save() override;

	//添加/删除 数据源
	virtual int addSource(Broadcast::ESourceType type,const char* srcName, const char *scene_name) override;
	virtual int removeSource(const char *srcName) override;

	//判断某个/某种类型 数据源是否存在 
	virtual bool hasSource(const char *srcName) override;
	virtual bool hasSource(Broadcast::ESourceType type)override;

	virtual Broadcast::ESourceType getSourceTypeByName(const char* srcName) override;
	virtual void enumSource(std::function<bool(Broadcast::ESourceType type, const char *src_name)> callback)override;
	//操控源在场景中的渲染效果
	virtual int manipulateSource(const char *srcName, Broadcast::ESourceActiton operation) override;
	//开始/停止预览 单个视频数据源预览
	virtual bool startPreviewSource(const char *srcName, const void* Window) override;
	virtual bool stopPreviewSource(const char *srcName) override;

	// 获得场景源管理类 预览场景相关接口
	virtual std::shared_ptr<ISceneManager> sceneManager()override;

	// 推流相关接口
	int startStream(const char *url, const char* key) override;
	int stopStream(bool bForce)override;
	virtual int getOutputSize(int &width, int &height) override;
	virtual int setOutputSize(int width, int height) override;

	/**************** 录制相关接口 ******************/
	// 获取/设置 录制文件保存路径
	virtual int setRecordFilePath(const char *file_path) override;
	virtual const char* getRecordFilePath() override;

	// 获取/设置 录制文件格式
	virtual int setRecordFileFormat(const char *recFmt) override;
	virtual const char * getRecordFileFormat() override;

	// 开始/结束 录制
	virtual int startRecord() override;
	virtual int stopRecord() override;

public:
	IBroadcastEngineObserver *engineObserver() 
	{ 
		return m_pEngineObserver; 
	}
	ConfigFile& Config() 
	{ 
		return m_globalConfigFile; 
	}

	int lookupByName(const std::string & name) 
	{ 
		return m_lookupByName[name];
	}

	std::string lookupByType(int type)
	{
		return m_lookupByType[type];
	}

	std::shared_ptr<SourceManager> sourceManger() 
	{
		return m_sourceManager; 
	}
	std::shared_ptr<SceneManager> scene_manager();
	OBSData getBaseData() { return m_baseDATA; }
protected:
	int createAudioInputSource(const char *src_name);
	int createAudioOutputSource(const char *src_name);
private:
	bool initGlobalConfig();
	std::string getSettingFilePath(const std::string &file_name);
	bool  loadSourceFromSetting(const std::string &file_name);
	bool  saveSourceToSetting(size_t flag);
	Broadcast::ESourceType getSourceType(const char *id);
private:
	IBroadcastEngineObserver *				m_pEngineObserver = nullptr;
	std::shared_ptr<ISceneManager>			m_sceneManager;
	std::shared_ptr<SourceManager>			m_sourceManager;
	ConfigFile								m_globalConfigFile;
	std::unordered_map<int, std::string>	m_lookupByType;
	std::unordered_map<std::string, int>	m_lookupByName;
	OBSData									m_baseDATA;
	std::unique_ptr<BasicOutputHandler>		m_outputHandler;
};

extern  BroadcastEngineImpl * g_pBroadcastEngine;