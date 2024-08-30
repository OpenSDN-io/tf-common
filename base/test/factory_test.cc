/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

#include "base/factory.h"

#include <boost/functional/factory.hpp>
#include <boost/scoped_ptr.hpp>

#include "testing/gunit.h"

class TypeA {
  public:
    TypeA() : value_(0) {}
    virtual ~TypeA(){}
  private:
    int value_;
};

class TypeB {
  public:
    TypeB() : value_(1) { }
    int value() const { return value_; }
    virtual ~TypeB(){}
  private:
    int value_;
};

class TypeAImpl1 : public TypeA {
  public:
    TypeAImpl1():TypeA(){};
};

class TypeAImpl2 : public TypeA {
  public:
    TypeAImpl2():TypeA(){};
};

class TypeBImpl1 : public TypeB {
  public:
    TypeBImpl1() : TypeB(){}
};

class TypeBImpl2 : public TypeB {
  public:
    TypeBImpl2() : TypeB(){}
};

class TypeX {
  public:
    TypeX(int value) : value_(value) { }
    virtual ~TypeX() { }
    const int value() const { return value_; }
  private:
    int value_;
};

class TypeY {
  public:
    TypeY(int x, int y) : x_(x), y_(x) { }

  private:
    int x_;
    int y_;
};

class TypeZ {
  public:
    TypeZ(int value) : value_(value + 1) { }
    const int value() const { return value_; }

  private:
    int value_;
};

class TypeW {
  public:
    TypeW(const TypeX *ptr) : ptr_(ptr) {
    }
  private:
    const TypeX *ptr_;
};

class TypeXImpl : public TypeX {
  public:
    TypeXImpl(int v): TypeX(v){}
};

class TypeXImpl1 : public TypeX {
  public:
    TypeXImpl1(int v): TypeX(v){}
};

class TypeXImpl2 : public TypeX {
  public:
    TypeXImpl2(int v): TypeX(v){}
};

struct TestStaticObjectFactory : public StaticObjectFactory {
};

using TypeARec = TestStaticObjectFactory::
    FactoryRecord<TypeA>;
using TypeBRec = TestStaticObjectFactory::
    FactoryRecord<TypeB>;
using TypeXRec = TestStaticObjectFactory::
    FactoryRecord<TypeX, int>;
using TypeYRec = TestStaticObjectFactory::
    FactoryRecord<TypeY, int, int>;
using TypeZRec = TestStaticObjectFactory::
    FactoryRecord<TypeZ, int>;
using TypeWRec = TestStaticObjectFactory::
    FactoryRecord<TypeW, const TypeX*>;

//Define function to create a new instance of TypeA
template<> TypeARec::FunctionType
    TypeARec::create_func_ = nullptr;
template<> TypeARec::DefaultLinkType
    TypeARec::default_link_{}; // the default type w/ default ctor for TypaA

template<> TypeBRec::FunctionType
    TypeBRec::create_func_ = nullptr;
template<> TypeBRec::DefaultLinkType
    TypeBRec::default_link_{};


template<> TypeXRec::FunctionType
    TypeXRec::create_func_ = nullptr;
template<> TypeYRec::FunctionType
    TypeYRec::create_func_ = nullptr;
template<> TypeZRec::FunctionType
    TypeZRec::create_func_ = nullptr;
template<> TypeWRec::FunctionType
    TypeWRec::create_func_ = nullptr;

template<> struct TestStaticObjectFactory::ParameterCastTo<TypeB,1>{
    using ImplType = TypeBImpl1;};
template<> struct TestStaticObjectFactory::ParameterCastTo<TypeB,2>{
    using ImplType = TypeBImpl2;};

template<> struct TestStaticObjectFactory::ParameterCastTo<TypeX,1>{
    using ImplType = TypeXImpl1;};
template<> struct TestStaticObjectFactory::ParameterCastTo<TypeX,2>{
    using ImplType = TypeXImpl2;};

class FactoryTest : public ::testing::Test {
  protected:
};

TEST_F(FactoryTest, Basic) {

    TestStaticObjectFactory::LinkImpl<TypeX,TypeX,
        int>();
    TestStaticObjectFactory::LinkImpl<TypeY,TypeY,
        int, int>();
    TestStaticObjectFactory::LinkImpl<TypeZ,TypeZ,
        int>();
    TestStaticObjectFactory::LinkImpl<TypeW,TypeW,
        const TypeX* >();

    boost::scoped_ptr<TypeB> b(TestStaticObjectFactory::Create<TypeB>());
    EXPECT_EQ(1, b->value());
    boost::scoped_ptr<TypeX> x(TestStaticObjectFactory::Create<TypeX>(1));
    EXPECT_EQ(1, x->value());
    boost::scoped_ptr<TypeY> y(TestStaticObjectFactory::Create<TypeY>(2, 3));
    boost::scoped_ptr<TypeW> w(TestStaticObjectFactory::Create<TypeW>(static_cast<const TypeX*>(x.get())));
}

TEST_F(FactoryTest, CheckLinkImpl) {
    {
        boost::scoped_ptr<TypeA> pA(TestStaticObjectFactory::Create<TypeA>());
        EXPECT_EQ(typeid(TypeA), typeid(*(pA.get())));
    }
    TestStaticObjectFactory::LinkImpl<TypeA,TypeAImpl1>();
    {
        boost::scoped_ptr<TypeA> pA(TestStaticObjectFactory::Create<TypeA>());
        EXPECT_EQ(typeid(TypeAImpl1), typeid(*(pA.get())));
    }
    TestStaticObjectFactory::LinkImpl<TypeA,TypeAImpl2>();
    {
        boost::scoped_ptr<TypeA> pA(TestStaticObjectFactory::Create<TypeA>());
        EXPECT_EQ(typeid(TypeAImpl2), typeid(*(pA.get())));
    }
    TestStaticObjectFactory::LinkImpl<TypeX,TypeX,int>();
    {
        boost::scoped_ptr<TypeX> pX(TestStaticObjectFactory::Create<TypeX>(10));
        EXPECT_EQ(typeid(TypeX), typeid(*(pX.get())));
    }
    TestStaticObjectFactory::LinkImpl<TypeX,TypeXImpl,int>();
    {
        boost::scoped_ptr<TypeX> pX(TestStaticObjectFactory::Create<TypeX>(10));
        EXPECT_EQ(typeid(TypeXImpl), typeid(*(pX.get())));
    }
}


TEST_F(FactoryTest, CheckParameterFactory) {
    {
        boost::scoped_ptr<TypeB> pB(TestStaticObjectFactory::Create<TypeB,1>());
        EXPECT_EQ(typeid(TypeBImpl1), typeid(*(pB.get())));
    }
    {
        boost::scoped_ptr<TypeB> pB(TestStaticObjectFactory::Create<TypeB,2>());
        EXPECT_EQ(typeid(TypeBImpl2), typeid(*(pB.get())));
    }
    {
        boost::scoped_ptr<TypeX> pX(TestStaticObjectFactory::Create<TypeX,1>(10));
        EXPECT_EQ(typeid(TypeXImpl1), typeid(*(pX.get())));
    }
    {
        boost::scoped_ptr<TypeX> pX(TestStaticObjectFactory::Create<TypeX,2>(10));
        EXPECT_EQ(typeid(TypeXImpl2), typeid(*(pX.get())));
    }
}

int main(int argc, char *argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
