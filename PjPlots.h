#include <array>
#include <vector>
#include <span>
#include <string_view>
#include <variant>
#include <exception>
#include <stdexcept>
#include <stdint.h>
#include <numeric>



namespace PjPlot {


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

#ifdef PJPLOT_ENABLE_TESTS
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
        static constexpr size_t length() {return Length;}
        static constexpr size_t nele() {return Length;}
        static constexpr size_t dims = 1;
    };

    template <size_t Rows, size_t Cols>
    struct StaticSize2 {
        using is_static_size = std::true_type;
        using is_size_type = std::true_type;
        static constexpr size_t rows() {return Rows;}
        static constexpr size_t cols() {return Cols;}
        static constexpr size_t nele() {return Rows*Cols;}
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
        static constexpr size_t slices() {return Slices;}
        static constexpr size_t rows() {return Rows;}
        static constexpr size_t cols() {return Cols;}
        static constexpr size_t nele() {return Slices*Rows*Cols;}
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

        static constexpr size_t nele() {return std::accumulate(Sizes.begin(), Sizes.end(), 1, std::multiplies<size_t>());}
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

        constexpr DynamicSize1(size_t length) : m_length(length) {}

        size_t length() {return m_length;}
        size_t nele() {return m_length;}
        size_t m_length = 0;
        static constexpr size_t dims = 1;
        using SliceType = void;
    };

    struct DynamicSize2 {
        static constexpr bool is_dynamic = true;
        using is_static_size = std::false_type;
        using is_size_type = std::true_type;

        constexpr DynamicSize2(size_t rows, size_t cols) : m_rows(rows), m_cols(cols) {}

        constexpr size_t rows() {return m_rows;}
        constexpr size_t cols() {return m_cols;}
        constexpr size_t nele() {return rows()*cols();}
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

        constexpr DynamicSize3(size_t slices, size_t rows, size_t cols) : m_slices(slices), m_rows(rows), m_cols(cols) {}

        constexpr size_t slices() {return m_slices;}
        constexpr size_t rows() {return m_rows;}
        constexpr size_t cols() {return m_cols;}
        constexpr size_t nele() {return slices()*rows()*cols();}
        size_t m_slices = 0;
        size_t m_rows = 0;
        size_t m_cols = 0;
        static constexpr size_t dims = 3;
        using SliceType = DynamicSize1;
        [[nodiscard]] auto slice() const noexcept {
            return DynamicSize2(m_rows, m_cols);
        }
    };
    
    template <size_t N>
    requires (N > 3)
    struct DynamicSizeN {
        using is_static_size = std::true_type;
        using is_size_type = std::true_type;
        constexpr DynamicSizeN(std::array<size_t, N>&& sizes) : m_sizes(sizes) {}

        constexpr size_t nele() {return std::accumulate(m_sizes.begin(), m_sizes.end(), 1, std::multiplies<size_t>());}
        std::array<size_t, N> m_sizes;
        static constexpr size_t dims = N;

        using SliceType = std::conditional_t<
            (N == 4),
            DynamicSize3,
            DynamicSizeN<N-1>                      // Dynamic size: std::vector
        >;

        [[nodiscard]] auto slice() const noexcept {
            if constexpr (N == 4) {
                return DynamicSize3(m_sizes[1], m_sizes[2], m_sizes[3]);
            } else {
                return DynamicSizeN(slice_size_array(m_sizes));
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

    /**
     * @brief Represents a failure message in the application.
     *
     * The `FailureType` class encapsulates a failure message, which can be used to provide context about errors
     * that occur during operations. This class is typically used in conjunction with `ResultWithValue`.
     */
    class FailureType {
    public:
        /**
         * @brief Default constructor for `FailureType`.
         */
        constexpr FailureType() {}

        /**
         * @brief Constructs a `FailureType` with a movable string message.
         * 
         * @param message The failure message, which will be moved into the class.
         */
        constexpr explicit FailureType(std::string&& message) : m_message(std::move(message)) {}

        /**
         * @brief Constructs a `FailureType` with a string view message.
         * 
         * @param message The failure message as a `std::string_view`, which will be copied into the class.
         */
        constexpr FailureType(std::string_view message) : m_message(message) {}

        /**
         * @brief Retrieves the failure message.
         * 
         * @return const std::string& The stored failure message.
         */
        [[nodiscard]] constexpr auto get_message() noexcept -> const std::string& {
            return m_message;
        }

    private:
        std::string m_message; ///< The failure message stored in the class.
    };

    /**
     * @brief Exception class for handling unhandled failures in `ResultWithValue`.
     *
     * The `UnhandledFailureException` class provides a way to signal that a failure occurred
     * when attempting to retrieve a value from a `ResultWithValue` instance, without the
     * caller explicitly handling the failure case.
     */
    class UnhandledFailureException : public std::exception {
    public:
        /**
         * @brief Constructs an `UnhandledFailureException` with a specified message.
         * 
         * @param message The message that describes the failure.
         */
        explicit UnhandledFailureException(const std::string& message)
            : m_message(message) {}

        /**
         * @brief Overrides the what() function to provide the error message.
         * 
         * @return const char* A C-style string containing the error message.
         */
        const char* what() const noexcept override {
            return m_message.c_str();
        }

    private:
        std::string m_message; ///< The failure message associated with the exception.
    };

    /**
     * @brief Represents a result that can either hold a value or indicate a failure.
     *
     * The `ResultWithValue` class can be used to return results from functions while
     * handling potential failure cases. The result can either be a successful value of type T
     * or a `FailureType` indicating the reason for failure.
     *
     * @tparam T The type of the value that can be returned.
     */
    template <typename T>
    class ResultWithValue {
    public:
        /**
         * @brief Constructs a `ResultWithValue` with a successful value.
         * 
         * @param value The successful value to be stored.
         */
        constexpr ResultWithValue(T&& value) : m_value(std::move(value)) {}

        /**
         * @brief Constructs a `ResultWithValue` with a failure indication.
         * 
         * @param value The `FailureType` indicating failure.
         */
        constexpr ResultWithValue(FailureType&& value) : m_value(std::move(value)) {}

        /**
         * @brief Factory method to create a failure result.
         * 
         * @param msg The message associated with the failure.
         * @return ResultWithValue<T> A `ResultWithValue` that indicates failure.
         */
        [[nodiscard]] constexpr static auto Failure(std::string&& msg) noexcept -> ResultWithValue<T> {
            return ResultWithValue<T>(FailureType(std::move(msg)));
        }

        /**
         * @brief Retrieves the stored value, or throws an exception if a failure occurred.
         * 
         * @return T& The stored value of type T.
         * @throws UnhandledFailureException If the stored value is a failure indication.
         */
        [[nodiscard]] constexpr auto get_value() -> T& {
            if (std::holds_alternative<FailureType>(m_value)) {
                // Extract the failure message and throw a UnhandledFailureException
                auto& failure = std::get<FailureType>(m_value);
                throw UnhandledFailureException(failure.get_message());
            }
            return std::get<T>(m_value);
        }

        /**
         * @brief Retrieves the stored value, or throws an exception if a failure occurred (const version).
         * 
         * @return const T& The stored value of type T.
         * @throws UnhandledFailureException If the stored value is a failure indication.
         */
        [[nodiscard]] constexpr auto get_value() const -> const T& {
            if (std::holds_alternative<FailureType>(m_value)) {
                // Extract the failure message and throw a UnhandledFailureException
                auto& failure = std::get<FailureType>(m_value);
                throw UnhandledFailureException(failure.get_message());
            }
            return std::get<T>(m_value);
        }

    private:
        std::variant<T, FailureType> m_value; ///< Holds either the successful value or a failure indication.
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
    template <typename T, SizeN Size, bool IsOwning = true>
    using StorageType = std::conditional_t<
        IsOwning, // If owning, select std::array or std::vector
        std::conditional_t<
            (Size::is_static_size::value),
            std::array<T, get_static_nele<Size>()>, // Static size: std::array
            std::vector<T>                          // Dynamic size: std::vector
        >,
        std::span<T> // If not owning, use std::span for both static and dynamic sizes
    >;

    template <typename T, SizeN Size, bool IsOwning = true>
    class ArrayNd {
        using is_static_size = Size::is_static_size;
    public:
        using is_array_type = std::true_type;

        // SFINAE constructor for when a static size type is provided
        constexpr ArrayNd(Size size) noexcept 
        requires (is_static_size::value && IsOwning) : m_size(size), m_data() {}

        // SFINAE constructor for when a dynamic size type is provided
        constexpr ArrayNd(Size size) noexcept 
        requires (!is_static_size::value && IsOwning) : m_size(size), m_data(std::vector<T>(size.nele())) {}

        // SFINAE constructor for when a view (non-owning) is created
        constexpr ArrayNd(Size size, T* data) noexcept 
        requires (!IsOwning) : m_size(size), m_data(std::span<T>(data, data +size.nele())) {}

        constexpr void resize(Size new_size) 
        requires (IsOwning && !is_static_size::value) {
            m_size = new_size;
            m_data.resize(new_size.nele());
        }

        [[nodiscard]] constexpr auto to_string() const noexcept -> std::string_view {

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
            return PjPlot::to_string<T>();
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
            static_assert(sizeof...(args) == m_size.dims, "Incorrect number of arguments");
            return m_data[calculate_linear_idx(m_size, args...)];
        }        

        // Element-wise access operator (const)
        template <typename... Args>
        [[nodiscard]] auto operator()(Args... args) const -> const T& {
            static_assert(sizeof...(args) == m_size.dims, "Incorrect number of arguments");
            return m_data[calculate_linear_idx(m_size, args...)];
        }

        [[nodiscard]] constexpr auto operator[](size_t idx) -> decltype(auto) {
            if constexpr (Size::dims > 1) {
                auto slice_size = m_size.slice();
                // return a contiguous view to the slice at the next lowest dimension
                return ArrayNd<T, typename Size::SliceType, false>(slice_size, m_data.data() + idx * slice_size.nele());
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
                return ArrayNd<const T, typename Size::SliceType, false>(slice_size, m_data.data() + idx * slice_size.nele());
            } else {
                // return reference to single value at the given index
                const T& ref = m_data[idx];
                return ref;
            }
        }

    protected:
        Size m_size;
        StorageType<T, Size, IsOwning> m_data;
    };

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

    enum class Colour {
        WHITE, BLACK, COUNT
    };


    template <typename T, Size3 Size>
    using Mat3View = ArrayNd<T, Size, false>;

    template <Colour Val>
        requires (Val == Colour::WHITE || Val == Colour::BLACK)
    [[nodiscard]] constexpr static auto to_string() -> std::string_view {
        if constexpr (Val == Colour::WHITE) {
            return "white";
        } else if constexpr (Val == Colour::BLACK) {
            return "black";
        }
    }

#ifdef PJPLOT_ENABLE_TESTS
    // little compile-time test to ensure all colour types are handled
    template <size_t I = 0>
    consteval static auto test_colour_to_string() -> bool{
        if constexpr (I < static_cast<size_t>(Colour::COUNT)) {
            constexpr auto s = to_string<static_cast<Colour>(I)>();
            return test_colour_to_string<I+1>();
        }
        return true;
    }
    constexpr static bool to_colour_to_string_test = test_colour_to_string();
#endif

    // runtime version 
    [[nodiscard]] static auto to_string(Colour val) -> std::string_view {
        switch (val) {
            case Colour::WHITE:
                return to_string<Colour::WHITE>();
            case Colour::BLACK:
                return to_string<Colour::BLACK>();
            default:
                // important for catching UB
                throw std::invalid_argument("Error: unsupported colour type");
        }
    }

    // a class to store the image elements for the grid, including lines, labels and ticks
    // each element has a pair of x and y coordinates (representing the top-left corner), and a Mat2 of RGBA values
    // the purpose is to quickly draw the grid on the image without having to iterate over the entire image
    // can support either dynamic memory or static memory for each of the grid elements depending on user requirements
    template <Size2 TitleSize, Size2 LabelSize, Size2 TickSize>
    class GridData {
    public:

    private:
        template <typename T>
        struct GridElement {
            T m_element;
            Vec2<size_t> m_offset;
        };
        static constexpr size_t k_max_num_labels = 2;
        static constexpr size_t k_max_num_ticks = 20;
        GridElement<Mat2<RGBA, TitleSize>> m_title;
        GridElement<ArrayNd<Mat2<RGBA, LabelSize>, StaticSize1<k_max_num_labels>>> m_labels;
        GridElement<ArrayNd<Mat2<RGBA, TickSize>, StaticSize1<k_max_num_ticks>>> m_ticks;
    };

    // a class to store the options for the grid, including whether to show x, y, x labels and y labels
    class GridOptions {
    public:
        constexpr GridOptions() = default;


        constexpr GridOptions(size_t border_pixels, bool show_x, bool show_y, bool show_x_labels, bool show_y_labels, bool show_minor_gridlines, bool show_major_gridlines) 
        : m_border_pixels(border_pixels), m_show_x(show_x), m_show_y(show_y), m_show_x_labels(show_x_labels), m_show_y_labels(show_y_labels), m_show_minor_gridlines(show_minor_gridlines), m_show_major_gridlines(show_major_gridlines) {

        }

        constexpr void set_border_pixels(size_t border_pixels) {
            m_border_pixels = border_pixels;
        }

        constexpr void set_show_x(bool show_x) {
            m_show_x = show_x;
        }

        constexpr void set_show_y(bool show_y) {
            m_show_y = show_y;
        }

        constexpr void set_show_x_labels(bool show_x_labels) {
            m_show_x_labels = show_x_labels;
        }

        constexpr void set_show_y_labels(bool show_y_labels) {
            m_show_y_labels = show_y_labels;
        }

        constexpr void set_show_minor_gridlines(bool show_minor_gridlines) {
            m_show_minor_gridlines = show_minor_gridlines;
        }

        constexpr void set_show_major_gridlines(bool show_major_gridlines) {
            m_show_major_gridlines = show_major_gridlines;
        }

        [[nodiscard]] constexpr auto get_show_x() const noexcept -> bool {
            return m_show_x;
        }

        [[nodiscard]] constexpr auto get_show_y() const noexcept -> bool {
            return m_show_y;
        }

        [[nodiscard]] constexpr auto get_show_x_labels() const noexcept -> bool {
            return m_show_x_labels;
        }

        [[nodiscard]] constexpr auto get_show_y_labels() const noexcept -> bool {
            return m_show_y_labels;
        }

        [[nodiscard]] constexpr auto get_show_minor_gridlines() const noexcept -> bool {
            return m_show_minor_gridlines;
        }

        [[nodiscard]] constexpr auto get_show_major_gridlines() const noexcept -> bool {
            return m_show_major_gridlines;
        }   

    private:

        size_t m_border_pixels = 0;
        bool m_show_x = true;
        bool m_show_y = true;
        bool m_show_x_labels = true;
        bool m_show_y_labels = true;
        bool m_show_minor_gridlines = true;
        bool m_show_major_gridlines = true;
    };  

    class AppearanceOptions {
    public:
        constexpr AppearanceOptions(){}

        [[nodiscard]] constexpr static auto create(Colour background, Colour text) noexcept-> ResultWithValue<AppearanceOptions> {
            if (background < Colour::COUNT && text < Colour::COUNT) {
                return ResultWithValue<AppearanceOptions>(AppearanceOptions(background, text));
            } else {
                return ResultWithValue<AppearanceOptions>::Failure("Error: invalid colour type");
            }
        }
        constexpr void set_background_colour(Colour background) {
            m_background_colour = background;
        }

        constexpr void set_text_colour(Colour text) {
            m_text_colour = text;
        }

        [[nodiscard]] constexpr auto get_background_colour() const noexcept {
            return m_background_colour;
        }

        [[nodiscard]] constexpr auto get_text_colour() const noexcept {
            return m_text_colour;
        }

    private:

        constexpr AppearanceOptions(Colour background, Colour text) 
        : m_background_colour(background), m_text_colour(text) {

        }

        Colour m_background_colour = Colour::WHITE;
        Colour m_text_colour = Colour::BLACK;
    };


    enum class ChartType {
        LINE, BAR, SCATTER, COUNT
    };

    template <ChartType Type>
        requires (Type < ChartType::COUNT) // valid chart type
    class Chart{
    public: 
        class Params {
        public:
            constexpr Params(size_t series_length, size_t num_series) 
            : m_series_length(series_length), m_num_series(num_series) {

            }
        private:
            size_t m_series_length;
            size_t m_num_series;
        };

        template <UnderlyingType ElementType, Size2 OutSize>
        [[nodiscard]] constexpr static auto get_plot(std::span<const ElementType> plot_data, Params params, const AppearanceOptions&, OutSize out_size) -> Img2<OutSize> {
            return Img2<OutSize>(out_size);
        }

        template <UnderlyingType ElementType, Size2 OutSize>
        constexpr static auto get_plot(std::span<const ElementType> plot_data, Params params, const AppearanceOptions&, Img2<OutSize>&) -> void {
            return;
        }

        struct TypeMapper {
            using type = Params;
        };
    };

    using LineChart = Chart<ChartType::LINE>;
    using ScatterChart = Chart<ChartType::SCATTER>;
    using BarChart = Chart<ChartType::BAR>;

    template <class PlotType>
    class plot_params_t {
    public:
        using type = typename PlotType::TypeMapper::type;
    };

    class Factory {
    public:
        constexpr Factory() {

        }
        template <class PlotType, UnderlyingType ElementType, Size2 OutSize = DynamicSize2>
        [[nodiscard]] constexpr auto get_plot(std::span<const ElementType> plot_data, typename plot_params_t<PlotType>::type params, OutSize output_size) const -> Img2<OutSize> {
            return PlotType::template get_plot<ElementType, OutSize>(plot_data, params, m_appearance_options, output_size);
        }
        
        template <class PlotType, UnderlyingType ElementType, Size2 OutSize = DynamicSize2>
        constexpr auto get_plot(std::span<const ElementType> plot_data, typename plot_params_t<PlotType>::type params, Img2<OutSize>& img_out) const -> void {
            return PlotType::template get_plot<ElementType, OutSize>(plot_data, params, m_appearance_options, img_out);
        }

        [[nodiscard]] constexpr auto get_appearance_options() noexcept -> AppearanceOptions& {
            return m_appearance_options;
        }

    private:
        AppearanceOptions m_appearance_options;
    };

}
