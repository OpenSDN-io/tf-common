/*
 * Copyright (c) 2019 Juniper Networks, Inc. All rights reserved.
 */

#include <string>
#include <base/feature_flags.h>
#include <assert.h>
#include <map>
#include <iostream>
#include <sstream>

using namespace std;
using contrail_rapidjson::Document;
using contrail_rapidjson::Value;

namespace process {

bool debug_ = true;

#define FLAG_DEBUG(args...) if (process::debug_) LOG(DEBUG, ##args)
#define FLAG_WARN(args...) if (process::debug_) LOG(WARN, ##args)
#define FLAG_ERROR(args...) if (process::debug_) LOG(ERROR, ##args)

/**
 * ------------------------------
 * Flag Class Method Implementations
 * ------------------------------
 */

/**
 * Constructors/Destructor
 */
Flag::Flag(FlagManager *manager, const string &name,
           const string &description, bool enabled,
           ContextVec &context_infos)
    : name_(name),
      description_(description),
      enabled_(enabled),
      context_infos_(context_infos),
      flag_state_cb_(NULL),
      manager_(manager)
{
    /**
      * Add flag to FlagManager's capability list
      */
    manager_->Register(this);
}

Flag::Flag(const Flag& flag, FlagStateCb callback)
    : name_(flag.name_),
      description_(flag.description_),
      enabled_(flag.enabled_),
      context_infos_(flag.context_infos_),
      flag_state_cb_(callback),
      manager_(flag.manager_)
{
    /**
     * Add flag to FlagManager's interest list
     */
    manager_->Register(this);
}

Flag::~Flag() {
    if (manager_) {
        manager_->Unregister(this);
    }
}

/**
 * Member Functions
 */

void Flag::InvokeCb() {
    if (!flag_state_cb_.empty()) {
        flag_state_cb_();
    }
}

bool Flag::operator == (const Flag &rhs) const {
    if (!(name_ == rhs.name_))
        return false;
    if (!(description_ == rhs.description_))
        return false;
    if (!(enabled_ == rhs.enabled_))
        return false;
    if (!(context_infos_ == rhs.context_infos_))
        return false;
    return true;
}

bool Flag::operator != (const Flag &rhs) const {
  return !(*this == rhs);
}

/**
 * ---------------------------------------------
 * FlagUveManager Class Method Implementations
 * ----------------------------------------------
 */

boost::scoped_ptr<FlagUveManager> FlagUveManager::instance_;

FlagUveManager::FlagUveManager(FlagManager *manager,
                               FlagUveCb flag_uve_cb)
    : flag_manager_(manager),
      flag_uve_cb_(flag_uve_cb) {
}

/**
 * CreateInstance should be called from ConnectionStateManager::Init()
 */
void FlagUveManager::CreateInstance(FlagManager *manager,
                                    FlagUveCb flag_uve_cb) {
    /**
     * The assert is to catch errors where FlagManager::GetInstance()
     * is called before ConnectionStateManager::Init()
     */
    assert(instance_ == NULL);
    instance_.reset(new FlagUveManager(manager, flag_uve_cb));
}

FlagUveManager* FlagUveManager::GetInstance() {
    if (instance_ == NULL) {
        instance_.reset(new FlagUveManager(NULL, NULL));
    }
    return instance_.get();
}

void FlagUveManager::SendUVE() {
    if (!flag_uve_cb_.empty()) {
        flag_uve_cb_();
    } else {
        FLAG_WARN("SendUVE cb not set");
    }
}

FlagConfigVec FlagUveManager::GetFlagInfos(bool lock) const {
    if (lock) {
        return (flag_manager_->GetFlagInfos());
    } else {
        return (flag_manager_->GetFlagInfosUnlocked());
    }
}

/**
 * -------------------------------------
 * FlagManager Class Method Implementations
 * --------------------------------------
 */

/**
 * FlagConfigManager instance
 */
boost::scoped_ptr<FlagConfigManager> FlagConfigManager::instance_;
string FlagConfigManager::version_;

FlagConfigManager::FlagConfigManager(FlagManager* manager)
    : flag_manager_(manager) {
}

/**
 * Initialize should be called from modules (for instance from main)
 */
void FlagConfigManager::Initialize(const string &build_info) {
    if (instance_ == NULL) {
        instance_.reset(new FlagConfigManager(FlagManager::GetInstance()));
    }

    Document d;
    if ( d.Parse<0>(build_info.c_str()).HasParseError() ) {
        FLAG_WARN("Parsing error");
    } else {
        const Value &v = d["build-info"];
        version_ = v[0]["build-version"].GetString();
    }
}

FlagConfigManager* FlagConfigManager::GetInstance() {
    if (instance_ == NULL) {
        instance_.reset(new FlagConfigManager(FlagManager::GetInstance()));
    }
    return instance_.get();
}

void FlagConfigManager::Set(const string &name,
                            const string &version, bool enabled,
                            FlagState::Type state,
                            ContextVec &context_infos) {
    /**
     * Call FlagManager to add/update the configuration for this feature flag
     * to its flag store if the version matches the module version.
     * Otherwise ignore.
     */
    if (version_ != version) {
        FLAG_DEBUG("Flag " << name << "'s version " << version <<
            " does not match with module version " << version_ <<
            " Ignoring.");
        return;
    }
    flag_manager_->Set(name, version, enabled, state, context_infos);
    FlagUveManager::GetInstance()->SendUVE();
}

void FlagConfigManager::Unset(const string &name) {
    /**
     * User unconfigured the flag. Call FlagManager to remove the
     * user configuration for this feature flag from its flag store.
     */
    flag_manager_->Unset(name);
    FlagUveManager::GetInstance()->SendUVE();
}

/**
 * -------------------------------------
 * FlagManager Class Method Implementations
 * --------------------------------------
 */

/**
 * FlagManager instance
 */
boost::scoped_ptr<FlagManager> FlagManager::instance_;

FlagManager* FlagManager::GetInstance() {
    if (instance_ == NULL) {
        instance_.reset(new FlagManager());
    }
    return instance_.get();
}

void FlagManager::ClearFlags() {
    flag_map_.clear();
}

size_t FlagManager::GetFlagMapCount() const {
    return (flag_map_.size());
}

void FlagManager::Set(const string &name, const string &version,
                      bool enabled, FlagState::Type state,
                      ContextVec &context_infos) {
    /**
     * Create a FlagConfig instance and add it to FlagMap if not present.
     * If already present, update it.
     */
    context_iterator c_itr;
    bool changed = false;

    FlagConfig flag_cfg(name, version, enabled, state, context_infos);

    std::scoped_lock lock(mutex_);
    flag_map_itr fmap_itr = flag_map_.find(name);
    if (fmap_itr != flag_map_.end()) {
        /**
         * Flag already present in FlagMap.
         * Check if something changed and update.
         */
        FlagConfig old_cfg = fmap_itr->second;

        FLAG_DEBUG("Flag " << name << "Already Present\n");
        FLAG_DEBUG(" EXISTING INFO:\n "
             << " Version: " << old_cfg.version()
             << " Enabled: " << string(old_cfg.enabled() ? "True " : "False ")
             << " State: " << FlagState::ToString(old_cfg.state()));
        const ContextVec c_vec = old_cfg.context_infos();
        FLAG_DEBUG("Context Info: ");
        for (c_itr = c_vec.begin(); c_itr != c_vec.end(); ++c_itr) {
            FLAG_DEBUG("Description: " << c_itr->desc
                       << " Value: " << c_itr->value);
        }

        if (old_cfg != flag_cfg) {
            FLAG_DEBUG("Flag " << name << " updated\n");
            changed = true;
            fmap_itr->second = flag_cfg;
        } else {
            FLAG_DEBUG("No change in flag " << name << " configuration\n");
        }
    } else {
        /**
         * New feature flag. Insert into FlagMap
         */
        changed = true;
        FLAG_DEBUG("New flag configured. Name: " << name);
        flag_map_.insert(make_pair(name, flag_cfg));
    }

    FLAG_DEBUG(" NEW INFO:\n "
      << " Version: " << flag_cfg.version()
      << " Enabled: " << string(flag_cfg.enabled() ? "True " : "False ")
      << " State: " << FlagState::ToString(flag_cfg.state()));
    const ContextVec c_vec = flag_cfg.context_infos();
    FLAG_DEBUG("Context Info: ");
    for (c_itr = c_vec.begin(); c_itr != c_vec.end(); ++c_itr) {
        FLAG_DEBUG("Description: " << c_itr->desc
                   << " Value: " << c_itr->value);
    }

    /**
     * New feature flag configured or change in existing flag definition.
     * If module is interested in this flag,
     * 1. invoke all callbacks registered by the module for this flag.
     * 2. Set enabled property of Flag in InterestMap so that when modules
     *    using this flag query to check if it is enabled, the updated
     *    result is returned.
     */
    if (changed) {
        bool enabled;
        pair <int_map_itr, int_map_itr> ret;
        ret = int_map_.equal_range(name);
        for (int_map_itr it = ret.first; it != ret.second; ++it) {
            enabled = false;
            Flag *f = it->second;
            enabled = IsFlagEnabled(name, f->enabled(), f->context_infos());
            f->set_enabled(enabled);
            f->InvokeCb();
        }
    }
}

void FlagManager::Unset(const string &name) {
    /**
     * Remove from FlagMap
     */
    flag_map_.erase(name);

    /**
     * Inform modules of the change
     */
    pair <int_map_itr, int_map_itr> ret;
    ret = int_map_.equal_range(name);
    for (int_map_itr it = ret.first; it != ret.second; ++it) {
        Flag *f = it->second;
        f->set_enabled(false);
        f->InvokeCb();
    }
}

/**
 * Check if Feature Flag is enabled given the name and
 * optional ContextInfo.
 * Feature is enabled under following conditions
 * 1. Feature flag present in FlagMap and is enabled OR
 * 2. Feature flag present in FlagMap but is not enabled
 *    However, default value is set to enabled AND
 * 3. If context is provided and matches what is configured
 *    in the FlagMap.
 * If feature is not present in the FlagMap meaning that user
 * has not configured the flag, the modules can use their own
 * default values.
 */
bool FlagManager::IsFlagEnabled(const string &name,
                                bool default_state,
                                const ContextVec &c_vec) const {
    flag_map_citr fitr;
    bool result = false;
    context_iterator c_itr, f_itr;

    FLAG_DEBUG("Checking if flag with name: " << name
               << " is enabled for context");
    for (c_itr = c_vec.begin(); c_itr != c_vec.end(); ++c_itr) {
        FLAG_DEBUG("Description: " << c_itr->desc
                   << " Value: " << c_itr->value);
    }

    fitr = flag_map_.find(name);
    if (fitr != flag_map_.end()) {
        /**
         * Get flag config
         */
        FlagConfig flag_cfg = fitr->second;

        /**
         * Check if Flag is enabled in config
         */
        if (flag_cfg.enabled()) {
            result = true;
        }

        /**
         * Check if context matches user config
         */
        const ContextVec f_vec = flag_cfg.context_infos();
        if (c_vec.empty() && !f_vec.empty()) {
            result = false;
        }
        for (c_itr = c_vec.begin(); c_itr != c_vec.end(); ++c_itr) {
            f_itr = find(f_vec.begin(), f_vec.end(), *c_itr);
            if (f_itr == f_vec.end()) {
                /**
                 *Context does not match
                 */
                result = false;
            }
        }
    } else {
        /**
         * Flag not present in FlagMap. Modules will use their
         * default state.
         */
        result = default_state;
    }

    FLAG_DEBUG("Flag enabled: " << string(result ? "True" : "False"));

    return result;
}

void FlagManager::Register(Flag *flag) {
    int_map_itr itr;
    flag_map_citr fitr;

    std::scoped_lock lock(mutex_);

    string name = flag->name();

    FLAG_DEBUG("Module interested in flag. Name: " << name <<
               " \nAdding to InterestMap");

    /**
     * Set value of flag if already present in FlagMap
     * Also invoke cb to let modules know about the presence of user
     * configuration for this flag
     */
    fitr = flag_map_.find(name);
    if (fitr != flag_map_.end()) {
        FLAG_DEBUG("Flag Name: " << name <<
                   " already present in FlagMap");
        bool value = IsFlagEnabled(name, flag->enabled(),
                                   flag->context_infos());
        flag->set_enabled(value);
        flag->InvokeCb();
    }

    /**
     * Insert into InterestMap
     */
    itr = int_map_.insert(make_pair(name, flag));
}

void FlagManager::Unregister(const Flag *flag) {
    pair <int_map_itr, int_map_itr> ret;
    const string name = flag->name();
    ret = int_map_.equal_range(name);

    FLAG_DEBUG("Module not interested in flag. Name: " << name <<
               " \nRemoving from InterestMap");

    std::scoped_lock lock(mutex_);

    int_map_itr it = ret.first;
    while (it != ret.second) {
        if (it->second == flag) {
            int_map_.erase(it);
            break;
        } else {
            ++it;
        }
    }
}

bool FlagManager::IsRegistered(const Flag *flag) const {
    pair <int_map_const_itr, int_map_const_itr> ret;

    std::scoped_lock lock(mutex_);

    ret = int_map_.equal_range(flag->name());
    for (int_map_const_itr it = ret.first; it != ret.second; ++it) {
        if (it->second == flag) {
            return true;
        }
    }
    return false;
}

size_t FlagManager::GetIntMapCount() const {
    return (int_map_.size());
}

FlagConfigVec FlagManager::GetFlagInfosUnlocked() const {
    flag_map_citr fitr;
    int_map_const_itr iitr;
    FlagConfigVec flag_infos;

    /**
     * Add flag only if module is interested. This information is
     * available in the InterestMap.
     */
    for (fitr = flag_map_.begin(); fitr != flag_map_.end(); ++fitr) {
        FlagConfig flag_cfg = fitr->second;
        iitr = int_map_.find(flag_cfg.name());
        if (iitr != int_map_.end()) {
            flag_infos.push_back(flag_cfg);
        }
    }

    return flag_infos;
}

FlagConfigVec FlagManager::GetFlagInfos() const {
    std::scoped_lock lock(mutex_);
    return GetFlagInfosUnlocked();
}

} // namespace process
