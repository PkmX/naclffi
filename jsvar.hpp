#ifndef JSVAR_HPP
#define JSVAR_HPP

#include <cstdint>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include <boost/variant.hpp>

#include "ppapi/cpp/var.h"
#include "ppapi/cpp/var_array.h"
#include "ppapi/cpp/var_dictionary.h"

template<typename InputIterator>
pp::VarArray make_vararray(const InputIterator first, const InputIterator last) {
    pp::VarArray va;
    std::uint32_t i = 0;
    std::for_each(first, last, [&](const auto v) { va.Set(i++, v); });
    return va;
}

pp::VarArray make_vararray(const std::initializer_list<pp::Var>& vs) {
    return make_vararray(std::begin(vs), std::end(vs));
}

pp::VarArray make_vararray(const pp::Var& v) {
    return make_vararray({v});
}

struct jsref {
    using id_type = std::int32_t;
    const std::shared_ptr<id_type> p;
    int32_t id() const { return *p; }
};

struct jsvar;

struct jsvar {
    jsvar() : var{pp::Var{}} {}
    jsvar(const pp::Var& x) : var{x} {}
    jsvar(const pp::VarArray& x) : var{x} {}
    jsvar(const pp::VarDictionary& x) : var{x} {}
    jsvar(const pp::Var::Null x) : var{pp::Var{x}} {}
    jsvar(const bool x) : var{pp::Var{x}} {}
    jsvar(const std::int32_t x) : var{pp::Var{x}} {}
    jsvar(const double x) : var{pp::Var{x}} {}
    jsvar(const std::string& x) : var{pp::Var{x}} {}
    jsvar(const std::vector<pp::Var>& x) : var{pp::VarArray{}} {
        std::uint32_t i = 0;
        for (const auto& v: x) {
            boost::get<pp::VarArray>(var).Set(i++, v);
        } 
    }
    jsvar(const std::unordered_map<std::string, pp::Var>& x) : var{pp::VarDictionary{}} {
        for (const auto& p: x) {
            boost::get<pp::VarDictionary>(var).Set(p.first, p.second);
        }
    }
    jsvar(const jsref& x) : var{x} {}

    using variant_type = boost::variant<pp::Var, pp::VarArray, pp::VarDictionary, jsref>;
    variant_type var;
};

#endif
