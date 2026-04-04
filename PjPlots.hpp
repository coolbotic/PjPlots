#include "utils.hpp"

namespace PjPlots {


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
        FailureType() = default;

        /**
         * @brief Constructs a `FailureType` with a movable string message.
         * 
         * @param message The failure message, which will be moved into the class.
         */
        explicit FailureType(std::string&& message) : m_message(std::move(message)) {}

        /**
         * @brief Constructs a `FailureType` with a string view message.
         * 
         * @param message The failure message as a `std::string_view`, which will be copied into the class.
         */
        FailureType(std::string_view message) : m_message(message) {}

        /**
         * @brief Retrieves the failure message.
         * 
         * @return const std::string& The stored failure message.
         */
        [[nodiscard]] auto get_message() noexcept -> const std::string& {
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

    enum class Colour {
        WHITE, BLACK, COUNT
    };


    template <typename T, Size3 Size>
    using Mat3View = ArrayNd<T, Size, false>;

    template <Colour Val>
        requires (Val < Colour::COUNT)
    [[nodiscard]] constexpr static auto to_string() -> std::string_view {
        if constexpr (Val == Colour::WHITE) {
            return "white";
        } else if constexpr (Val == Colour::BLACK) {
            return "black";
        } else {
            // static_assert(always_false<Colour>::value, "Error: unhandled type");
        }
    }

#ifdef PjPlots_ENABLE_TESTS
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