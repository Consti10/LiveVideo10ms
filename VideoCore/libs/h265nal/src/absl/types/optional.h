#ifndef MY_OPTIONAL_ABS
#define MY_OPTIONAL_ABS

#include <optional>

// the original h265nal library uses absl::optional since it cannot use c++17 where std::optional was introduced
// Since c++17 is available on android ndk, I bring std::optional into the absl::optional namespace
// such that the library compiles without absl
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
