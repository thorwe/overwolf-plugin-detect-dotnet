/*
  Overwolf NPAPI Plugin
  Copyright (c) 2014 Overwolf Ltd.
*/
#include "nsPluginInstancenpDetectDotNetPlugin.h"
#include "nsScriptableObjectnpDetectDotNetPlugin.h" // our specific API

// we use this to force our plugin container to shut down
// when no one is using it.  Browsers try to keep the plugin
// open for optimization reasons - we don't want it
int nsPluginInstancenpDetectDotNetPlugin::ref_count_ = 0;

////////////////////////////////////////
//
// nsPluginInstancenpDetectDotNetPlugin class implementation
//
nsPluginInstancenpDetectDotNetPlugin::nsPluginInstancenpDetectDotNetPlugin(NPP instance) :
  nsPluginInstanceBase(),
  instance_(instance),
  initialized_(FALSE),
  scriptable_object_(nullptr) {

  nsPluginInstancenpDetectDotNetPlugin::ref_count_++;
}

nsPluginInstancenpDetectDotNetPlugin::~nsPluginInstancenpDetectDotNetPlugin() {
  nsPluginInstancenpDetectDotNetPlugin::ref_count_--;

  if (0 == nsPluginInstancenpDetectDotNetPlugin::ref_count_) {
    PostQuitMessage(0);
  }
}

// NOTE:
// ------
// Overwolf plugins should not implement windows - NPAPI will
// probably be removed in the near feature and will be changed
// by a different method that will only support non-visual
// plugins
NPBool nsPluginInstancenpDetectDotNetPlugin::init(NPWindow* window) {
  // no GUI to init in windowless case
  initialized_ = TRUE;
  return TRUE;
}

void nsPluginInstancenpDetectDotNetPlugin::shut() {
  if (nullptr != scriptable_object_) {
    NPN_ReleaseObject(scriptable_object_);
  }

  initialized_ = FALSE;
}

NPBool nsPluginInstancenpDetectDotNetPlugin::isInitialized() {
  return initialized_;
}

// here we supply our scriptable object
NPError nsPluginInstancenpDetectDotNetPlugin::GetValue(
  NPPVariable variable, void* ret_value) {
  
  NPError rv = NPERR_INVALID_PARAM;

  switch (variable) {
    case NPPVpluginScriptableNPObject:
    {
      if (nullptr == scriptable_object_) {
        scriptable_object_ = 
          NPN_CreateObject(
            instance_, 
            GET_NPOBJECT_CLASS(nsScriptableObjectnpDetectDotNetPlugin));

        NPN_RetainObject(scriptable_object_);

        ((nsScriptableObjectnpDetectDotNetPlugin*)scriptable_object_)->Init();
        *(NPObject **)ret_value = scriptable_object_;
      }

      rv = NPERR_NO_ERROR;
      return rv;
    }
    default:
      break;
  }

  return rv;
}
