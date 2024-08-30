/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

#ifndef __BASE__FACTORY_H__
#define __BASE__FACTORY_H__

// in Boost this macro defaults to 6 but we're defining FACTORY_TYPE_N8
// so we need to define it manually
#define BOOST_FUNCTIONAL_FORWARD_ADAPTER_MAX_ARITY 8

#include <functional>
#include <iostream>

#include <boost/function.hpp>
#include <boost/functional/factory.hpp>
#include <boost/functional/forward_adapter.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/utility/enable_if.hpp>

#include "base/util.h"

struct StaticObjectFactory {

    template<class Base, class ... Args>
    struct FactoryTypes
    {
        using BaseType = Base;
        using BasePointer = BaseType *;
        using Signature = BasePointer(Args...);
        using FunctionType = std::function<Signature>;
    };

    template<class Base, class Impl>
    struct Creator {
        template<class ...Args> static
        typename FactoryTypes<Base,Args...>::BasePointer
        new_instance(Args &&... args) {
            return static_cast<typename
                FactoryTypes<Base,Args...>::BasePointer>
                (new Impl(args...));
        };
    };

    template<class Base, class Impl, class ... Args>
    static typename FactoryTypes<Base, Args...>::BasePointer
    NewInstance(Args ...args)
    {
        return static_cast<typename StaticObjectFactory::FactoryTypes
            <Base, Args...>::BasePointer> (new Impl (args...));
    };

    template<class Base, class ... Args>
    struct DefaultLink;

    template<class Base, class ... Args>
    struct FactoryRecord
    {
        using Signature = typename FactoryTypes<Base, Args...>::Signature;
        using FunctionType = typename FactoryTypes<Base, Args...>::FunctionType;
        using DefaultLinkType = DefaultLink<Base, Args...>;

        static FunctionType create_func_;
        static DefaultLinkType default_link_;
    };

    template<class Base, class Impl, class ... Args>
    static void LinkImpl()
    {
        FactoryRecord<Base, Args...>::create_func_ =
            StaticObjectFactory::NewInstance<Base, Impl, Args...>;
    };

    template<class Base, int p>
    struct ParameterCastTo {
    };

    template<class Base, class ... Args>
    struct DefaultLink
    {
        DefaultLink()
        {
            StaticObjectFactory::LinkImpl<Base, Base, Args...>();
        }
    };

    template<class Base, class ... Args>
    static typename FactoryTypes<Base, Args...>::BasePointer
    Create(Args ...args) {
        if (!bool(FactoryRecord<Base, Args...>::create_func_)) {
            return nullptr;
        }
        return FactoryRecord<Base, Args...>::create_func_(args...);
    };

    template<class BaseType, int par, class ...Args>
    static typename FactoryTypes<BaseType, Args...>::BasePointer
    Create(Args &&...args) {
        return Creator<
            BaseType,
            typename ParameterCastTo<BaseType,par>::ImplType>::new_instance(args...);
    };
};

#endif
