// RUN: %clang_cc1 -std=c++20 -verify %s

namespace PR47043 {
  template<typename T> concept True = true;
  template<typename ...T> concept AllTrue1 = True<T>; // expected-error {{expression contains unexpanded parameter pack 'T'}}
  template<typename ...T> concept AllTrue2 = (True<T> && ...);
  static_assert(AllTrue2<int, float, char>);
}

namespace PR47025 {
  template<typename ...T> concept AllAddable1 = requires(T ...t) { (void(t + 1), ...); };
  template<typename ...T> concept AllAddable2 = (requires(T ...t) { (t + 1); } && ...); // expected-error {{requirement contains unexpanded parameter pack 't'}}
  template<typename ...T> concept AllAddable3 = (requires(T t) { (t + 1); } && ...);
  template<typename ...T> concept AllAddable4 = requires(T t) { (t + 1); }; // expected-error {{expression contains unexpanded parameter pack 'T'}}
  template<typename ...T> concept AllAddable5 = requires(T t) { (void(t + 1), ...); }; // expected-error {{does not contain any unexpanded}}
  template<typename ...T> concept AllAddable6 = (requires { (T() + 1); } && ...);
  template<typename ...T> concept AllAddable7 = requires { (T() + 1); }; // expected-error {{expression contains unexpanded parameter pack 'T'}}

  static_assert(AllAddable1<int, float>);
  static_assert(AllAddable3<int, float>);
  static_assert(AllAddable6<int, float>);
  static_assert(!AllAddable1<int, void>);
  static_assert(!AllAddable3<int, void>);
  static_assert(!AllAddable6<int, void>);
}

namespace PR45699 {
  template<class> concept C = true; // expected-note 2{{here}}
  template<class ...Ts> void f1a() requires C<Ts>; // expected-error {{requires clause contains unexpanded parameter pack 'Ts'}}
  template<class ...Ts> requires C<Ts> void f1b(); // expected-error {{requires clause contains unexpanded parameter pack 'Ts'}}
  template<class ...Ts> void f2a() requires (C<Ts> && ...);
  template<class ...Ts> requires (C<Ts> && ...) void f2b();
  template<class ...Ts> void f3a() requires C<Ts...>; // expected-error {{pack expansion used as argument for non-pack parameter of concept}}
  template<class ...Ts> requires C<Ts...> void f3b(); // expected-error {{pack expansion used as argument for non-pack parameter of concept}}
  template<class ...Ts> void f4() {
    ([] () requires C<Ts> {} ()); // expected-error {{expression contains unexpanded parameter pack 'Ts'}}
    ([]<int = 0> requires C<Ts> () {} ()); // expected-error {{expression contains unexpanded parameter pack 'Ts'}}
  }
  template<class ...Ts> void f5() {
    ([] () requires C<Ts> {} (), ...);
    ([]<int = 0> requires C<Ts> () {} (), ...);
  }
  void g() {
    f1a();
    f1b(); // FIXME: Bad error recovery. expected-error {{undeclared identifier}}
    f2a();
    f2b();
    f3a();
    f3b(); // FIXME: Bad error recovery. expected-error {{undeclared identifier}}
    f4();
    f5();
  }
}

namespace P0857R0 {
  void f() {
    auto x = []<bool B> requires B {}; // expected-note {{constraints not satisfied}} expected-note {{false}}
    x.operator()<true>();
    x.operator()<false>(); // expected-error {{no matching member function}}
  }

  // FIXME: This is valid under P0857R0.
  template<typename T> concept C = true;
  template<template<typename T> requires C<T> typename U> struct X {}; // expected-error {{requires 'class'}} expected-error 0+{{}}
  template<typename T> requires C<T> struct Y {};
  X<Y> xy; // expected-error {{no template named 'X'}}
}

namespace PR50306 {
  template<typename T> concept NotInt = sizeof(T) != sizeof(int); // expected-note {{because}}
  template<typename T> void f() {
    [](NotInt auto) {}(T()); // expected-error {{no matching function}} expected-note {{constraints not satisfied}} expected-note {{because}}
  }
  template void f<char>(); // OK
  template void f<int>(); // expected-note {{in instantiation of}}
}

namespace PackInTypeConstraint {
  template<typename T, typename U> concept C = sizeof(T) == sizeof(int); // expected-note 3{{}}

  template<typename ...T, C<T> U> void h1(); // expected-error {{type constraint contains unexpanded parameter pack 'T'}}
  template<typename ...T, C<T> ...U> void h2();
  template<typename ...T> void h3(C<T> auto); // expected-error {{type constraint contains unexpanded parameter pack 'T'}}
  template<typename ...T> void h4(C<T> auto...);

  template<typename ...T> void f1() {
    []<C<T> U>(U u){}(T()); // expected-error {{unexpanded parameter pack 'T'}}
  }
  template<typename ...T> void f2() {
    ([]<C<T> U>(U u){}(T()), ...); // expected-error {{no match}} expected-note 2{{}}
  }
  template void f2<int, int, int>(); // OK
  template void f2<int, char, double>(); // expected-note {{in instantiation of}}
  void f3() {
    ([]<typename ...T, C<T> U>(U u){}(0), // expected-error {{type constraint contains unexpanded parameter pack 'T'}}
     ...); // expected-error {{does not contain any unexpanded}}
  }

  template<typename ...T> void g1() {
    [](C<T> auto){}(T()); // expected-error {{expression contains unexpanded parameter pack 'T'}}
  }
  template<typename ...T> void g2() {
    ([](C<T> auto){}(T()), ...); // expected-error {{no matching function}} expected-note {{constraints not satisfied}} expected-note {{because}}
  }
  template void g2<int, int, int>(); // OK
  template void g2<int, char, double>(); // expected-note {{in instantiation of}}
  void g3() {
    ([]<typename ...T>(C<T> auto){}(1), // expected-error {{type constraint contains unexpanded parameter pack 'T'}}
     ...); // expected-error {{does not contain any unexpanded}}
  }

  template<typename ...T> void g4() {
    []() -> C<T> auto{ return T(); }(); // expected-error {{expression contains unexpanded parameter pack 'T'}}
  }
  template<typename ...T> void g5() {
    ([]() -> C<T> auto{ // expected-error-re {{deduced type {{.*}} does not satisfy}}
     return T();
     }(), ...);
  }
  template void g5<int, int, int>(); // OK
  template void g5<int, char, double>(); // expected-note {{in instantiation of}}
  void g6() {
    ([]<typename ...T>() -> C<T> auto{ // expected-error {{declaration type contains unexpanded parameter pack 'T'}}
     return T(); // expected-error {{expression contains unexpanded parameter pack 'T'}}
     }(),
     ...); // expected-error {{does not contain any unexpanded}}
  }
}

namespace BuiltinIsConstantEvaluated {
  // Check that we do all satisfaction and diagnostic checks in a constant context.
  template<typename T> concept C = __builtin_is_constant_evaluated(); // expected-warning {{always}}
  static_assert(C<int>);

  template<typename T> concept D = __builtin_is_constant_evaluated() == true; // expected-warning {{always}}
  static_assert(D<int>);

  template<typename T> concept E = __builtin_is_constant_evaluated() == true && // expected-warning {{always}}
                                   false; // expected-note {{'false' evaluated to false}}
  static_assert(E<int>); // expected-error {{failed}} expected-note {{because 'int' does not satisfy 'E'}}

  template<typename T> concept F = __builtin_is_constant_evaluated() == false; // expected-warning {{always}}
  // expected-note@-1 {{'__builtin_is_constant_evaluated() == false' (1 == 0)}}
  static_assert(F<int>); // expected-error {{failed}} expected-note {{because 'int' does not satisfy 'F'}}

  template<typename T> concept G = __builtin_is_constant_evaluated() && // expected-warning {{always}}
                                   false; // expected-note {{'false' evaluated to false}}
  static_assert(G<int>); // expected-error {{failed}} expected-note {{because 'int' does not satisfy 'G'}}
}

namespace NoConstantFolding {
  // Ensure we use strict constant evaluation rules when checking satisfaction.
  int n;
  template <class T> concept C = &n + 3 - 3 == &n; // expected-error {{non-constant expression}} expected-note {{cannot refer to element 3 of non-array object}}
  static_assert(C<void>); // expected-note {{while checking}}
}

namespace PR50337 {
  template <typename T> concept foo = true;
  template <typename T> concept foo2 = foo<T> && true;
  void f(foo auto, auto);
  void f(foo2 auto, auto);
  void g() { f(1, 2); }
}

namespace PR50561 {
  template<typename> concept C = false;
  template<typename T, typename U> void f(T, U);
  template<C T, typename U> void f(T, U) = delete;
  void g() { f(0, 0); }
}

namespace PR49188 {
  template<class T> concept C = false;     // expected-note 7 {{because 'false' evaluated to false}}

  C auto f1() { // expected-error {{deduced type 'void' does not satisfy 'C'}}
    return void();
  }
  C auto f2() { // expected-error {{deduced type 'void' does not satisfy 'C'}}
    return;
  }
  C auto f3() { // expected-error {{deduced type 'void' does not satisfy 'C'}}
  }
  C decltype(auto) f4() { // expected-error {{deduced type 'void' does not satisfy 'C'}}
    return void();
  }
  C decltype(auto) f5() { // expected-error {{deduced type 'void' does not satisfy 'C'}}
    return;
  }
  C decltype(auto) f6() { // expected-error {{deduced type 'void' does not satisfy 'C'}}
  }
  C auto& f7() { // expected-error {{deduced type 'void' does not satisfy 'C'}}
    return void();
  }
  C auto& f8() {
    return; // expected-error {{cannot deduce return type 'C auto &' from omitted return expression}}
  }
  C auto& f9() { // expected-error {{cannot deduce return type 'C auto &' for function with no return statements}}
  }
}
namespace PR53911 {
  template<class T> concept C = false; // expected-note 3 {{because 'false' evaluated to false}}

  C auto *f1() { // expected-error {{deduced type 'void' does not satisfy 'C'}}
    return (void*)nullptr;
  }
  C auto *f2() { // expected-error {{deduced type 'int' does not satisfy 'C'}}
    return (int*)nullptr;
  }
  C auto *****f3() { // expected-error {{deduced type 'int' does not satisfy 'C'}}
    return (int*****)nullptr;
  }
}

namespace PR54379 {
template <int N>
struct A {
  static void f() requires (N == 0) { return; } // expected-note {{candidate template ignored: constraints not satisfied}} expected-note {{evaluated to false}}
  static void f() requires (N == 1) { return; } // expected-note {{candidate template ignored: constraints not satisfied}} expected-note {{evaluated to false}}
};
void (*f1)() = A<2>::f; // expected-error {{address of overloaded function 'f' does not match required type}}

struct B {
  template <int N2 = 1> static void f() requires (N2 == 0) { return; }  // expected-note {{candidate template ignored: constraints not satisfied [with N2 = 1]}} expected-note {{evaluated to false}}
};
void (*f2)() = B::f; // expected-error {{address of overloaded function 'f' does not match required type}}
}

namespace PR54443 {

template <class T, class U>
struct is_same { static constexpr bool value = false; };

template <class T>
struct is_same<T, T> { static constexpr bool value = true; };

template <class T, class U>
concept same_as = is_same<T, U>::value; // expected-note-re 4 {{because {{.*}} evaluated to false}}

int const &f();

same_as<int const> auto i1 = f(); // expected-error {{deduced type 'int' does not satisfy 'same_as<const int>'}}
same_as<int const> auto &i2 = f();
same_as<int const> auto &&i3 = f(); // expected-error {{deduced type 'const int &' does not satisfy 'same_as<const int>'}}

same_as<int const &> auto i4 = f(); // expected-error {{deduced type 'int' does not satisfy 'same_as<const int &>'}}
same_as<int const &> auto &i5 = f(); // expected-error {{deduced type 'const int' does not satisfy 'same_as<const int &>'}}
same_as<int const &> auto &&i6 = f();

template <class T>
concept C = false; // expected-note 3 {{because 'false' evaluated to false}}

int **const &g();

C auto **j1 = g();   // expected-error {{deduced type 'int' does not satisfy 'C'}}
C auto **&j2 = g();  // expected-error {{deduced type 'int' does not satisfy 'C'}}
C auto **&&j3 = g(); // expected-error {{deduced type 'int' does not satisfy 'C'}}
}

namespace DeferredInstantiationInstScope {
template <typename T>
struct remove_ref {
  using type = T;
};
template <typename T>
struct remove_ref<T &> {
  using type = T;
};
template <typename T>
struct remove_ref<T &&> {
  using type = T;
};

template <typename T>
constexpr bool IsInt = PR54443::is_same<typename remove_ref<T>::type,
                                        int>::value;

template <typename U>
void SingleDepthReferencesTop(U &&u) {
  struct lc {
    void operator()()             // #SDRT_OP
      requires IsInt<decltype(u)> // #SDRT_REQ
    {}
  };
  lc lv;
  lv(); // #SDRT_CALL
}

template <typename U>
void SingleDepthReferencesTopNotCalled(U &&u) {
  struct lc {
    void operator()()
      requires IsInt<typename decltype(u)::FOO>
    {}
  };
  lc lv;
}

template <typename U>
void SingleDepthReferencesTopCalled(U &&u) {
  struct lc {
    void operator()()                           // #CALLOP
      requires IsInt<typename decltype(u)::FOO> // #CONSTR
    {}
  };
  lc lv;
  lv();
  // expected-error@-1{{no matching function for call to object of type 'lc'}}
  // expected-note@#SDRTC{{in instantiation of function template}}
  // expected-note@#CALLOP{{constraints not satisfied}}
  // expected-note@#CONSTR{{substituted constraint expression is ill-formed}}
}

template <typename U>
void SingleDepthReferencesTopLambda(U &&u) {
  []()
    requires IsInt<decltype(u)>
  {}();
}

template <typename U>
void DoubleDepthReferencesTop(U &&u) {
  struct lc { // #DDRT_STRCT
    void operator()() {
      struct lc2 {
        void operator()()             // #DDRT_OP
          requires IsInt<decltype(u)> // #DDRT_REQ
        {}
      };
      lc2 lv2;
      lv2(); // #DDRT_CALL
    }
  };
  lc lv;
  lv();
}

template <typename U>
void DoubleDepthReferencesTopLambda(U &&u) {
  []() { []()
           requires IsInt<decltype(u)>
         {}(); }();
}

template <typename U>
void DoubleDepthReferencesAll(U &&u) {
  struct lc { // #DDRA_STRCT
    void operator()(U &&u2) {
      struct lc2 {
        void operator()(U &&u3)          // #DDRA_OP
          requires IsInt<decltype(u)> && // #DDRA_REQ
                   IsInt<decltype(u2)> && IsInt<decltype(u3)>
        {}
      };
      lc2 lv2;
      lv2(u2); // #DDRA_CALL
    }
  };
  lc lv;
  lv(u);
}

template <typename U>
void DoubleDepthReferencesAllLambda(U &&u) {
  [](U &&u2) {
    [](U && u3)
      requires IsInt<decltype(u)> &&
               IsInt<decltype(u2)> && IsInt<decltype(u3)>
    {}(u2);
  }(u);
}

template <typename U>
void HasInnerFunc(U &&u) {
  void InnerFunc(U && u2)
    requires IsInt<decltype(u)> && // #INNERFUNC_REQ
             IsInt<decltype(u2)>;
  InnerFunc(u); // #INNERFUNC_CALL
}

template <typename U>
struct CausesFriendConstraint {
  template <typename V>
  friend void FriendFunc(CausesFriendConstraint, V) // #FF_DECL
    requires IsInt<U> &&
             IsInt<V> // #FF_REQ
  {}
};
// FIXME: Re-enable this test when constraints are allowed to refer to captures.
// template<typename T>
// void ChecksCapture(T x) {
//   [y = x]() requires(IsInt<decltype(y)>){}();
// }

template <typename T>
void ChecksLocalVar(T x) {
  T Local;
  []()
    requires(IsInt<decltype(Local)>)
  {}();
}

template <typename T>
void LocalStructMemberVar(T x) {
  struct S {
    T local;
    void foo()
      requires(IsInt<decltype(local)>) // #LSMV_REQ
    {}
  } s;
  s.foo(); // #LSMV_CALL
};

template <typename T>
struct ChecksMemberVar {
  T t;
  void foo()
    requires(IsInt<decltype(t)>) // #CMV_FOO
  {}
  template <typename U>
  void foo2()                    // #CMV_FOO2
    requires(IsInt<decltype(t)>) // #CMV_FOO2_REQ
  {}
};

void test_dependent() {
  int v = 0;
  float will_fail;
  SingleDepthReferencesTop(v);
  SingleDepthReferencesTop(will_fail);
  // expected-error@#SDRT_CALL{{no matching function for call to object of type 'lc'}}
  // expected-note@-2{{in instantiation of function template specialization}}
  // expected-note@#SDRT_OP{{candidate function not viable}}
  // expected-note@#SDRT_REQ{{'IsInt<decltype(u)>' evaluated to false}}

  SingleDepthReferencesTopNotCalled(v);
  // Won't error unless we try to call it.
  SingleDepthReferencesTopNotCalled(will_fail);
  SingleDepthReferencesTopCalled(v); // #SDRTC
  SingleDepthReferencesTopLambda(v);
  // FIXME: This should error on constraint failure! (Lambda!)
  SingleDepthReferencesTopLambda(will_fail);
  DoubleDepthReferencesTop(v);
  DoubleDepthReferencesTop(will_fail);
  // expected-error@#DDRT_CALL{{no matching function for call to object of type 'lc2'}}
  // expected-note@-2{{in instantiation of function template specialization}}
  // expected-note@#DDRT_STRCT{{in instantiation of member function}}
  // expected-note@#DDRT_OP{{candidate function not viable}}
  // expected-note@#DDRT_REQ{{'IsInt<decltype(u)>' evaluated to false}}

  DoubleDepthReferencesTopLambda(v);
  // FIXME: This should error on constraint failure! (Lambda!)
  DoubleDepthReferencesTopLambda(will_fail);
  DoubleDepthReferencesAll(v);
  DoubleDepthReferencesAll(will_fail);
  // expected-error@#DDRA_CALL{{no matching function for call to object of type 'lc2'}}
  // expected-note@-2{{in instantiation of function template specialization}}
  // expected-note@#DDRA_STRCT{{in instantiation of member function}}
  // expected-note@#DDRA_OP{{candidate function not viable}}
  // expected-note@#DDRA_REQ{{'IsInt<decltype(u)>' evaluated to false}}

  DoubleDepthReferencesAllLambda(v);
  // FIXME: This should error on constraint failure! (Lambda!)
  DoubleDepthReferencesAllLambda(will_fail);
  HasInnerFunc(v);
  HasInnerFunc(will_fail);
  // expected-error@#INNERFUNC_CALL{{invalid reference to function 'InnerFunc': constraints not satisfied}}
  // expected-note@-2{{in instantiation of function template specialization}}
  // expected-note@#INNERFUNC_REQ{{'IsInt<decltype(u)>' evaluated to false}}

  CausesFriendConstraint<int> CFC;
  FriendFunc(CFC, 1);
  FriendFunc(CFC, 1.0);
  // expected-error@-1{{no matching function for call to 'FriendFunc'}}
  // expected-note@#FF_DECL{{constraints not satisfied}}
  // expected-note@#FF_REQ{{because 'IsInt<double>' evaluated to false}}

  // FIXME: Re-enable this test when constraints are allowed to refer to captures.
  // ChecksCapture(v);

  ChecksLocalVar(v);
  // FIXME: This should error on constraint failure! (Lambda!)
  ChecksLocalVar(will_fail);

  LocalStructMemberVar(v);
  LocalStructMemberVar(will_fail);
  // expected-error@#LSMV_CALL{{invalid reference to function 'foo'}}
  // expected-note@-2{{in instantiation of function template specialization}}
  // expected-note@#LSMV_REQ{{because 'IsInt<decltype(this->local)>' evaluated to false}}

  ChecksMemberVar<int> CMV;
  CMV.foo();
  CMV.foo2<int>();

  ChecksMemberVar<float> CMV2;
  CMV2.foo();
  // expected-error@-1{{invalid reference to function 'foo'}}
  // expected-note@#CMV_FOO{{because 'IsInt<decltype(this->t)>' evaluated to false}}
  CMV2.foo2<float>();
  // expected-error@-1{{no matching member function for call to 'foo2'}}
  // expected-note@#CMV_FOO2{{constraints not satisfied}}
  // expected-note@#CMV_FOO2_REQ{{because 'IsInt<decltype(this->t)>' evaluated to false}}
}
} // namespace DeferredInstantiationInstScope
