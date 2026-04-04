#include <array>
#include <vector>
#include <span>
#include <string_view>
#include <variant>
#include <exception>
#include <stdexcept>
#include <stdint.h>
#include <numeric>

namespace PjPlots {
    template <typename>
    struct always_false : std::false_type {};

    template <typename T>
    struct Vec2 {
        T x;
        T y;
    };

    template <typename T>
    struct Vec3 {
        T x;
        T y;
        T z;
    };

    struct RGB {
        uint8_t r;
        uint8_t g;
        uint8_t b;
    };

    struct RGBA {
        constexpr RGBA() = default;
        constexpr RGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a) 
        : m_r(r), m_g(g), m_b(b), m_a(a) {
        }
        uint8_t m_r;
        uint8_t m_g;
        uint8_t m_b;
        uint8_t m_a;
    };

    using v_AllowedTypes = std::variant<int, uint8_t, uint32_t, float, double, RGBA>;

    // Concept to constrain T to one of the allowed types in v_AllowedTypes
    // Helper trait to check if a type is in a variant
    template <typename T, typename Variant>
    struct is_in_variant;

    // Specialization to recursively check each type in the variant
    template <typename T, typename... Types>
    struct is_in_variant<T, std::variant<Types...>> 
        : std::disjunction<std::is_same<T, Types>...> {};

    // Convenience variable template
    template <typename T, typename Variant>
    inline constexpr bool is_in_variant_v = is_in_variant<T, Variant>::value;

    template <typename T>
    concept AllowedType = is_in_variant_v<T, v_AllowedTypes>;
    
    // helper function to check if variant contains template parameter
    template <AllowedType T, typename V, size_t I = 0>
    constexpr static auto check_contains() -> bool {
        if constexpr (I < std::variant_size_v<V>) {
            constexpr bool ret = check_contains<T, V, I+1>();
            return std::is_same_v<std::variant_alternative_t<I, V>, T> || ret;
        }
        return false;
    }

    template <AllowedType T>
    [[nodiscard]] constexpr static auto to_string() -> std::string_view {
        static_assert(check_contains<T, v_AllowedTypes>());
        if constexpr (std::is_same_v<T, int>) {
            return "int";
        } else if constexpr (std::is_same_v<T, uint8_t>) {
            return "uint8_t";
        } else if constexpr (std::is_same_v<T, uint32_t>) {
            return "uint32_t";
        } else if constexpr (std::is_same_v<T, float>) {
            return "float";
        } else if constexpr (std::is_same_v<T, double>) {
            return "double";
        } else if constexpr (std::is_same_v<T, RGBA>) {
            return "RGBA";
        } else {
            static_assert(always_false<T>::value, "Error: unhandled type");
        }
    }

#ifdef PjPlots_ENABLE_TESTS
    // little compile-time test to ensure all variant types are handled
    template <size_t I = 0>
    consteval static auto test_to_string() -> bool{
        if constexpr (I < std::variant_size_v<v_AllowedTypes>) {
            constexpr auto s = to_string<std::variant_alternative_t<I, v_AllowedTypes>>();
            return test_to_string<I+1>();
        }
        return true;
    }
    constexpr static bool to_string_test = test_to_string();
#endif
    // convenience alias for use outside 'DataType'
    template <typename T>
    concept UnderlyingType = AllowedType<T>;


    template <size_t Length>
    struct StaticSize1 {
        using is_static_size = std::true_type;
        using is_size_type = std::true_type;
        static constexpr size_t length() noexcept {return Length;}
        static constexpr size_t nele() noexcept {return Length;}
        static constexpr size_t dims = 1;
    };

    template <size_t Rows, size_t Cols>
    struct StaticSize2 {
        using is_static_size = std::true_type;
        using is_size_type = std::true_type;
        static constexpr size_t rows() noexcept {return Rows;}
        static constexpr size_t cols() noexcept {return Cols;}
        static constexpr size_t nele() noexcept {return Rows*Cols;}
        static constexpr size_t dims = 2;
        using SliceType = StaticSize1<Cols>;
        [[nodiscard]] auto slice() const noexcept {
            return StaticSize1<Cols>();
        }
    };

    template <size_t Slices, size_t Rows, size_t Cols>
    struct StaticSize3 {
        using is_static_size = std::true_type;
        using is_size_type = std::true_type;
        static constexpr size_t slices() noexcept {return Slices;}
        static constexpr size_t rows() noexcept {return Rows;}
        static constexpr size_t cols() noexcept {return Cols;}
        static constexpr size_t nele() noexcept {return Slices*Rows*Cols;}
        static constexpr size_t dims = 3;
        using SliceType = StaticSize2<Rows, Cols>;
        [[nodiscard]] auto slice() const noexcept {
            return StaticSize2<Rows, Cols>();
        }
    };

    template <size_t N>
    constexpr inline auto slice_size_array(std::array<size_t, N> sizes) noexcept {
        std::array<size_t, N-1> res; 
        for (size_t i = 0; i < (N-1); ++i) 
            res[i] = sizes[i+1]; 
        return res;
    }

    template <size_t N, std::array<size_t, N> Sizes>
    requires (N > 3)
    struct StaticSizeN {
        using is_static_size = std::true_type;
        using is_size_type = std::true_type;

        static constexpr size_t nele() {return std::accumulate(Sizes.begin(), Sizes.end(), size_t(1), std::multiplies<size_t>());}
        static constexpr size_t dims = N;

        using SliceType = std::conditional_t<
            (N == 4),
            StaticSize3<Sizes[1], Sizes[2], Sizes[3]>,
            StaticSizeN<N-1, slice_size_array(Sizes)>
        >;
        [[nodiscard]] auto slice() const noexcept {
            return SliceType();
        }
    };

    struct DynamicSize1 {
        static constexpr bool is_dynamic = true;
        using is_static_size = std::false_type;
        using is_size_type = std::true_type;

        constexpr DynamicSize1() = default;
        constexpr DynamicSize1(size_t length) : m_length(length) {}

        constexpr size_t length() const noexcept {return m_length;}
        constexpr size_t nele() const noexcept {return m_length;}
        size_t m_length = 0;
        static constexpr size_t dims = 1;
        using SliceType = void;
        constexpr bool operator==(const DynamicSize1& other) const noexcept {
            return (m_length == other.m_length);
        }
    };

    struct DynamicSize2 {
        static constexpr bool is_dynamic = true;
        using is_static_size = std::false_type;
        using is_size_type = std::true_type;

        constexpr DynamicSize2() = default;
        constexpr DynamicSize2(size_t rows, size_t cols) : m_rows(rows), m_cols(cols) {}
        constexpr DynamicSize2(const DynamicSize2&) = default;
        constexpr DynamicSize2(DynamicSize2&&) = default;
        constexpr DynamicSize2& operator=(const DynamicSize2&) = default;
        constexpr DynamicSize2& operator=(DynamicSize2&&) = default;

        constexpr bool operator==(const DynamicSize2& other) const noexcept {
            return (m_rows == other.m_rows) && (m_cols == other.m_cols);
        }

        [[nodiscard]] static constexpr auto transpose(const DynamicSize2 & other) -> DynamicSize2 {
            return DynamicSize2(other.rows(), other.cols());
        }

        constexpr size_t rows() const noexcept {return m_rows;}
        constexpr size_t cols() const noexcept {return m_cols;}
        constexpr size_t nele() const noexcept {return rows()*cols();}
        size_t m_rows = 0;
        size_t m_cols = 0;
        static constexpr size_t dims = 2;
        using SliceType = DynamicSize1;
        [[nodiscard]] auto slice() const noexcept {
            return DynamicSize1(m_cols);
        }
    };

    struct DynamicSize3 {
        static constexpr bool is_dynamic = true;
        using is_static_size = std::false_type;
        using is_size_type = std::true_type;

        constexpr DynamicSize3() = default;
        constexpr DynamicSize3(size_t slices, size_t rows, size_t cols) : m_slices(slices), m_rows(rows), m_cols(cols) {}

        constexpr bool operator==(const DynamicSize3& other) const noexcept {
            return (m_rows == other.m_rows) && (m_cols == other.m_cols) && (m_slices == other.m_slices);
        }

        constexpr size_t slices() const noexcept {return m_slices;}
        constexpr size_t rows() const noexcept {return m_rows;}
        constexpr size_t cols() const noexcept {return m_cols;}
        constexpr size_t nele() const noexcept {return slices()*rows()*cols();}
        size_t m_slices = 0;
        size_t m_rows = 0;
        size_t m_cols = 0;
        static constexpr size_t dims = 3;
        using SliceType = DynamicSize2;
        [[nodiscard]] auto slice() const noexcept {
            return DynamicSize2(m_rows, m_cols);
        }
    };
    
    template <size_t N>
    requires (N > 0)
    struct DynamicSizeN {
        using is_static_size = std::false_type;
        using is_size_type = std::true_type;

        constexpr DynamicSizeN() = default;
        constexpr DynamicSizeN(std::array<size_t, N>&& sizes) : m_sizes(sizes) {}


        constexpr bool operator==(const DynamicSizeN<N>& other) const noexcept {
            return (m_sizes == other.m_sizes);
        }


        constexpr size_t nele() const noexcept {return std::accumulate(m_sizes.begin(), m_sizes.end(), size_t(1), std::multiplies<size_t>());}
        std::array<size_t, N> m_sizes;
        static constexpr size_t dims = N;

        template <size_t I>
        requires(I < N)
        [[nodiscard]] constexpr auto get() const noexcept -> size_t {
            return m_sizes[I];
        }
        using SliceType = std::conditional_t<
            N == 4,
            DynamicSize3,
            DynamicSizeN<N-1>
        >;
        
        [[nodiscard]] auto slice() const noexcept {
            if constexpr (N == 4) {
                return SliceType(m_sizes[1], m_sizes[2], m_sizes[3]);
            } else {
                return SliceType(slice_size_array(m_sizes));
            }
        }
    };

    template <typename T>
    concept Size1 = requires {
        typename T::is_size_type;  // Ensures the type has the is_size_type trait
        { std::declval<T>().length() } -> std::same_as<size_t>;
        T::dims == 1;
    };

    [[nodiscard]] inline constexpr auto calculate_linear_idx(Size1 auto dims, size_t idx) -> size_t {
        if constexpr (idx >= dims.length()) {
            throw std::out_of_range("Index out of range");
        }
        return idx;
    }

    // Define a concept that checks if a type is a size type
    template <typename T>
    concept Size2 = requires {
        typename T::is_size_type;  // Ensures the type has the is_size_type trait
        { std::declval<T>().cols() } -> std::convertible_to<size_t>;
        { std::declval<T>().rows() } -> std::convertible_to<size_t>;
        T::dims == 2;
    };

    [[nodiscard]] inline constexpr auto calculate_linear_idx(Size2 auto dims, size_t row, size_t col) -> size_t {
        if constexpr (row >= dims.rows()) {
            throw std::out_of_range("Row index out of range");
        }
        if constexpr (col >= dims.cols()) {
            throw std::out_of_range("Column index out of range");
        }
        const auto row_idx = row * dims.cols();
        return row_idx + col;
    }


    // Define a concept that checks if a type is a size type
    template <typename T>
    concept Size3 = requires {
        typename T::is_size_type;  // Ensures the type has the is_size_type trait
        { std::declval<T>().slices() } -> std::convertible_to<size_t>;
        { std::declval<T>().rows() } -> std::convertible_to<size_t>;
        { std::declval<T>().cols() } -> std::convertible_to<size_t>;
        T::dims == 3;
    };

    [[nodiscard]] inline constexpr auto calculate_linear_idx(Size3 auto dims, size_t slice, size_t row, size_t col) -> size_t {
        if constexpr (slice >= dims.slices()) {
            throw std::out_of_range("Slice index out of range");
        }
        if constexpr (row >= dims.rows()) {
            throw std::out_of_range("Row index out of range");
        }
        if constexpr (col >= dims.cols()) {
            throw std::out_of_range("Column index out of range");
        }
        const auto slice_idx = slice * dims.rows() * dims.cols();
        const auto row_idx = row * dims.cols();
        return slice_idx + row_idx + col;
    }


    // helper function to unpack an array of indices and call the correct overload of calculate_linear_idx
    // currently only used to generically constrain SizeN
    template <size_t N, typename T, size_t... Is>
    constexpr size_t unpack_and_calculate(const std::array<T, N>& indices, std::index_sequence<Is...>) {
        return calculate_linear_idx(indices[Is]...);  // Unpack the array elements
    }

    template <typename T>
    concept SizeN = requires {
        typename T::is_size_type;  // Ensures the type has the is_size_type trait
        { T::dims } -> std::convertible_to<size_t>;
        { std::declval<T>().nele() } -> std::convertible_to<size_t>;
        { unpack_and_calculate(std::array<size_t, T::dims>{}, std::make_index_sequence<T::dims>{}) } -> std::convertible_to<size_t>;
    };


    // helper function to enable inferrence of static size at compile time, used for inferring StorageType
    template <SizeN Size>
    constexpr auto get_static_nele() -> size_t {
        if constexpr (Size::is_static_size::value) {
            return Size::nele();
        } else {
            return 0;
        }
    }

    // Type trait to select storage type based on size (static or dynamic) and ownership (owning or non-owning)
    template <SizeN Size, typename DynamicStorageType, typename StaticStorageType>
    using OwningTypeSelection = std::conditional_t<
        (Size::is_static_size::value),
        StaticStorageType, // Static size: std::array
        DynamicStorageType                         // Dynamic size: std::vector
    >;

    // Type trait to select storage type based on size (static or dynamic) and ownership (owning or non-owning)
    template <typename T, bool IsOwning, typename OwningType>
    using StorageTypeSelection = std::conditional_t<
        IsOwning, // If owning, select std::array or std::vector
        OwningType,
        std::span<T> // If not owning, use std::span for all other types
    >;

    template <typename T, SizeN Size, bool IsOwning = true, typename OwningType = OwningTypeSelection<T, std::vector<T>, std::array<T, get_static_nele<Size>()>>>
    class ArrayNd {
        using is_static_size = typename Size::is_static_size;
    public:
        using storage_type_t = StorageTypeSelection<T, IsOwning, OwningType>;
        using is_array_type = std::true_type;
        using underlying_type_t = T;
        using size_type_t = Size;
        static constexpr bool k_is_owning = IsOwning;

        // SFINAE constructor for when a static size type is provided
        constexpr ArrayNd(Size size) noexcept 
        requires (is_static_size::value && IsOwning) : m_size(size), m_data() {}

        // SFINAE default constructor for resizable version
        constexpr ArrayNd() noexcept 
        requires (!is_static_size::value && IsOwning) : m_size(Size{}) {}

        // SFINAE constructor for when a dynamic size type is provided
        constexpr ArrayNd(Size size) 
        requires (!is_static_size::value && IsOwning) : m_size(size), m_data(storage_type_t(size.nele())) {}

        // SFINAE constructor for when a view (non-owning) is created
        constexpr ArrayNd(Size size, T* data) 
        requires (!IsOwning) : m_size(size), m_data(std::span<T>(data, data +size.nele())) {}

        // SFINAE constructor for moving data into new object, throws if sizes don't match
        constexpr ArrayNd(Size size, storage_type_t&& data) 
        requires (IsOwning) : m_size(size), m_data(std::move(data)) {
            if (size.nele() != data.size()) {
                throw std::invalid_argument("Error [ArrayNd]: size object and size of data doesn't match");
            }
        }

        constexpr void resize(Size new_size) 
        requires (IsOwning && !is_static_size::value) {
            m_size = new_size;
            m_data.resize(new_size.nele());
        }

        [[nodiscard]] constexpr static auto to_string() noexcept -> std::string_view {

            // Handle the case of dimensions from 1 to 9
            switch (Size::dims) {
                case 1: return "1-D array";
                case 2: return "2-D array";
                case 3: return "3-D array";
                case 4: return "4-D array";
                case 5: return "5-D array";
                case 6: return "6-D array";
                case 7: return "7-D array";
                case 8: return "8-D array";
                case 9: return "9-D array";
                default: return "N-D array";  // Default case for higher dimensions
            }
        }

        // Getter for the number of elements
        [[nodiscard]] constexpr auto nele() const noexcept -> size_t {
            return m_size.nele();
        }

        // Getter for the type as a string (using to_string)
        [[nodiscard]] constexpr auto type_s() const noexcept -> std::string_view {
            return PjPlots::to_string<T>();
        }

        // Const getter for array data as std::span
        [[nodiscard]] auto data() const noexcept -> std::span<const T> {
            return std::span<const T>(m_data.data(), m_data.size());
        }

        // Non-const getter for array data as std::span
        [[nodiscard]] auto data() noexcept -> std::span<T> {
            return std::span<T>(m_data.data(), m_data.size());
        }

        // Iterator support
        using iterator = T*;
        using const_iterator = const T*;

        // Begin and end functions for non-const and const access
        [[nodiscard]] constexpr auto begin() noexcept -> iterator {
            return m_data.data();
        }

        [[nodiscard]] constexpr auto end() noexcept -> iterator {
            return m_data.data() + m_data.size();
        }

        [[nodiscard]] constexpr auto begin() const noexcept -> const_iterator {
            return m_data.data();
        }

        [[nodiscard]] constexpr auto end() const noexcept -> const_iterator {
            return m_data.data() + m_data.size();
        }

        // Element-wise access operator (non-const)
        template <typename... Args>
        [[nodiscard]] inline auto operator()(Args... args) -> T& {
            static_assert(sizeof...(args) == Size::dims, "Incorrect number of arguments");
            return m_data[calculate_linear_idx(m_size, args...)];
        }        

        // Element-wise access operator (const)
        template <typename... Args>
        [[nodiscard]] auto operator()(Args... args) const -> const T& {
            static_assert(sizeof...(args) == Size::dims, "Incorrect number of arguments");
            return m_data[calculate_linear_idx(m_size, args...)];
        }

        [[nodiscard]] constexpr auto operator[](size_t idx) -> decltype(auto) {
            if constexpr (Size::dims > 1) {
                auto slice_size = m_size.slice();
                // return a contiguous view to the slice at the next lowest dimension
                return ArrayNd<T, typename Size::SliceType, false, OwningType>(slice_size, m_data.data() + idx * slice_size.nele());
            } else {
                // return reference to single value at the given index
                T& ref = m_data[idx];
                return ref;
            }
        }

        [[nodiscard]] constexpr auto operator[](size_t idx) const -> decltype(auto) {
            if constexpr (Size::dims > 1) {
                auto slice_size = m_size.slice();
                // return a contiguous view to the slice at the next lowest dimension
                return ArrayNd<const T, typename Size::SliceType, false, OwningType>(slice_size, m_data.data() + idx * slice_size.nele());
            } else {
                // return reference to single value at the given index
                const T& ref = m_data[idx];
                return ref;
            }
        }

        [[nodiscard]] bool set_data(auto&& new_data) {
            if (m_size.nele() != new_data.size()) {
                return false;
            }
            size_t count = 0;
            for (const auto & val : new_data) {
                m_data[count++] = val;
            }
            return true;        
        }

        [[nodiscard]] bool set_data(storage_type_t&& new_data)
        requires (IsOwning) {
            if (m_size.nele() != new_data.size()) {
                return false;
            }
            m_data = std::move(new_data);
            return true;
        } 

        [[nodiscard]] constexpr auto size() const noexcept -> const Size& {
            return m_size;
        }

    protected:
        Size m_size;
        storage_type_t m_data;
    };

    template <typename T, PjPlots::SizeN Size, typename StorageType>
    [[nodiscard]] constexpr inline auto create_view(PjPlots::ArrayNd<T, Size, true, StorageType> & packet) -> PjPlots::ArrayNd<T, Size, false, StorageType> {
        return PjPlots::ArrayNd<T, Size, false, StorageType>(packet.size(), packet.begin());
    }

    template <typename T, PjPlots::SizeN Size, typename StorageType>
    [[nodiscard]] constexpr inline auto create_packet(PjPlots::ArrayNd<T, Size, false, StorageType> view) -> PjPlots::ArrayNd<T, Size, true, StorageType> {
        auto res = PjPlots::ArrayNd<T, Size, true, StorageType>(view.size());
        if (!res.set_data(view.data())) {
            throw std::invalid_argument("invalid view");
        }
        return res;
    }


    template <typename T, Size1 Size>
    using Array1d = ArrayNd<T, Size>;

    template <typename T, Size2 Size>
    class Mat2 : public ArrayNd<T, Size> {
    public:

        constexpr Mat2(Size size) noexcept 
        : ArrayNd<T, Size>(size) {}

        // Getter for the number of columns
        [[nodiscard]] constexpr auto cols() const noexcept -> size_t {
            return ArrayNd<T, Size>::cols();
        }

        // Getter for the number of rows
        [[nodiscard]] constexpr auto rows() const noexcept -> size_t {
            return ArrayNd<T, Size>::rows();
        }
    };

    template <typename T, Size2 Size>
    using Mat2View = ArrayNd<T, Size, false>;

    template <typename T, Size3 Size>
    class Mat3 : public ArrayNd<T, Size> {
    public:

        constexpr Mat3(Size size) noexcept 
        : ArrayNd<T, Size>(size) {}

        // Getter for the number of slices
        [[nodiscard]] constexpr auto slices() const noexcept -> size_t {
            return ArrayNd<T, Size>::slices();
        }

        // Getter for the number of columns
        [[nodiscard]] constexpr auto cols() const noexcept -> size_t {
            return ArrayNd<T, Size>::cols();
        }

        // Getter for the number of rows
        [[nodiscard]] constexpr auto rows() const noexcept -> size_t {
            return ArrayNd<T, Size>::rows();
        }
    };

    // Alias for an image type (e.g., RGBA matrix)
    template <Size2 Size>
    using Img2 = Mat2<RGBA, Size>;

    template <Size2 Size>
    using Img2F = Mat2<float, Size>;
}
