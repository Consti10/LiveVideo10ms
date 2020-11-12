#ifndef MY_OPTIONAL_ABS
#define MY_OPTIONAL_ABS

#include <optional>

namespace absl{
    template<class T>
    using optional=std::optional<T>;

    //template<class T>
    //using nullopt=std::optional;
    //typedef decltype(nullopt) nullptr_t;
    //inline constexpr std::nullopt nullopt{/*unspecified*/};
    //struct nullopt_t{/* see description */};
    inline constexpr auto nullopt=std::nullopt;
}

#endif //MY_OPTIONAL_ABS
