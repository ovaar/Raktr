/*!
 * @file flag_set.h
 * @brief Type-safe class for using enums as flags with std::bitset.
 *
 * Based on https://github.com/mrts/flag-set-cpp
 * License: MIT
 */

#ifndef RAKTR_ENGINE_CORE_FLAG_SET_H
#define RAKTR_ENGINE_CORE_FLAG_SET_H

#include <bitset>
#include <cassert>
#include <iostream>
#include <string>

namespace raktr::engine
{

    /*!
     * @brief Type-safe class for using enums as flags with std::bitset.
     *
     * The enum must contain a sentinel value `_` as the last entry,
     * which is used to determine the bitset size.
     *
     * @example
     * enum class MyFlags {
     *     Flag1,
     *     Flag2,
     *     Flag3,
     *     _  // sentinel
     * };
     *
     * FlagSet<MyFlags> flags;
     * flags.set(MyFlags::Flag1);
     * if (flags[MyFlags::Flag1]) {
     *     // Flag1 is set
     * }
     */
    template <typename T>
    class FlagSet
    {
    public:
        FlagSet() = default;

        explicit FlagSet(const T& val)
        {
            _flags.set(static_cast<u_type>(val));
        }

        // Binary operations

        FlagSet& operator&=(const T& val) noexcept
        {
            bool tmp = _flags.test(static_cast<u_type>(val));
            _flags.reset();
            _flags.set(static_cast<u_type>(val), tmp);
            return *this;
        }

        FlagSet& operator&=(const FlagSet& o) noexcept
        {
            _flags &= o._flags;
            return *this;
        }

        FlagSet& operator|=(const T& val) noexcept
        {
            _flags.set(static_cast<u_type>(val));
            return *this;
        }

        FlagSet& operator|=(const FlagSet& o) noexcept
        {
            _flags |= o._flags;
            return *this;
        }

        // The resulting bitset can contain at most 1 bit
        FlagSet operator&(const T& val) const
        {
            FlagSet ret(*this);
            ret &= val;
            assert(ret._flags.count() <= 1);
            return ret;
        }

        FlagSet operator&(const FlagSet& val) const
        {
            FlagSet ret(*this);
            ret._flags &= val._flags;
            return ret;
        }

        // The resulting bitset contains at least 1 bit
        FlagSet operator|(const T& val) const
        {
            FlagSet ret(*this);
            ret |= val;
            assert(ret._flags.count() >= 1);
            return ret;
        }

        FlagSet operator|(const FlagSet& val) const
        {
            FlagSet ret(*this);
            ret._flags |= val._flags;
            return ret;
        }

        FlagSet operator~() const
        {
            FlagSet cp(*this);
            cp._flags.flip();
            return cp;
        }

        // The bitset evaluates to true if any bit is set
        explicit operator bool() const
        {
            return _flags.any();
        }

        // Methods from std::bitset

        bool operator==(const FlagSet& o) const
        {
            return _flags == o._flags;
        }

        std::size_t size() const
        {
            return _flags.size();
        }

        std::size_t count() const
        {
            return _flags.count();
        }

        FlagSet& set()
        {
            _flags.set();
            return *this;
        }

        FlagSet& reset()
        {
            _flags.reset();
            return *this;
        }

        FlagSet& flip()
        {
            _flags.flip();
            return *this;
        }

        FlagSet& set(const T& val, bool value = true)
        {
            _flags.set(static_cast<u_type>(val), value);
            return *this;
        }

        FlagSet& reset(const T& val)
        {
            _flags.reset(static_cast<u_type>(val));
            return *this;
        }

        FlagSet& flip(const T& val)
        {
            _flags.flip(static_cast<u_type>(val));
            return *this;
        }

        constexpr bool operator[](const T& val) const
        {
            return _flags[static_cast<u_type>(val)];
        }

        std::string to_string() const
        {
            return _flags.to_string();
        }

        // Operator for outputting to std::ostream
        friend std::ostream& operator<<(std::ostream& stream, const FlagSet& self)
        {
            return stream << self._flags;
        }

    private:
        using u_type = std::underlying_type_t<T>;

        // _ is last value sentinel and must be present in enum T
        std::bitset<static_cast<u_type>(T::_)> _flags;
    };

    template <typename T, typename = void>
    struct is_enum_that_contains_sentinel : std::false_type
    {
    };

    template <typename T>
    struct is_enum_that_contains_sentinel<T, decltype(static_cast<void>(T::_))> : std::is_enum<T>
    {
    };

    // Operator that combines two enumeration values into a FlagSet only if the
    // enumeration contains the sentinel `_`
    template <typename T>
    std::enable_if_t<is_enum_that_contains_sentinel<T>::value, FlagSet<T>>
    operator|(const T& lhs, const T& rhs)
    {
        FlagSet<T> fs;
        fs |= lhs;
        fs |= rhs;
        return fs;
    }

} // namespace raktr::engine

#endif // RAKTR_ENGINE_CORE_FLAG_SET_H
