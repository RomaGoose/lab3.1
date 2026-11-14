#include "avl_tree.hpp"
#include "console/generator.hpp"

#include <ftxui/component/app.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include <algorithm>
#include <array>
#include <charconv>
#include <limits>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include <unistd.h>

struct record {
    int id;
    std::string name;
};

struct record_compare {
    bool operator()(const record& left, const record& right) const;
};

bool record_compare::operator()(const record& left, const record& right) const {
    return left.id < right.id;
}

std::ostream& operator<<(std::ostream& stream, const record& value) {
    return stream << value.id << ": " << value.name;
}

std::optional<int> parse_number(std::string_view input) {
    auto first = input.find_first_not_of(" \t\n\r\f\v");
    if (first == std::string_view::npos) return std::nullopt;
    auto last = input.find_last_not_of(" \t\n\r\f\v");
    input = input.substr(first, last - first + 1);

    if (input.front() == '+') {
        input.remove_prefix(1);
        if (input.empty() || input.front() == '-' || input.front() == '+')
            return std::nullopt;
    }

    int value = 0;
    auto [end, error] = std::from_chars(input.data(), input.data() + input.size(), value);
    if (error != std::errc{} || end != input.data() + input.size())
        return std::nullopt;
    return value;
}

int main() {
    using namespace ftxui;

    if (!isatty(STDIN_FILENO)) return 0;

    avl_tree<int> numbers;
    avl_tree<record, record_compare> records;
    std::string number;
    std::string name;
    std::string count_text = "10";
    std::string minimum_text = "0";
    std::string maximum_text = "99";
    std::string seed_text = "42";
    std::string message = "Готово";
    bool input_error = false;
    std::vector<std::string> modes = {"Числа", "Записи"};
    int selected_mode = 0;
    std::vector<std::string> orders = {"КЛП", "КПЛ", "ЛПК", "ЛКП", "ПЛК", "ПКЛ"};
    const std::array order_values = {
        traversal_order::root_left_right, traversal_order::root_right_left,
        traversal_order::left_right_root, traversal_order::left_root_right,
        traversal_order::right_left_root, traversal_order::right_root_left
    };
    int selected_order = 3;

    auto read_value = [&] {
        auto value = parse_number(number);
        input_error = !value.has_value();
        if (input_error) {
            message = "Введите целое число от " +
                std::to_string(std::numeric_limits<int>::min()) + " до " +
                std::to_string(std::numeric_limits<int>::max()) + ".";
        }
        return value;
    };

    auto insert_value = [&] {
        auto value = read_value();
        if (!value) return;
        if (selected_mode == 1 &&
            name.find_first_not_of(" \t\n\r\f\v") == std::string::npos) {
            input_error = true;
            message = "Введите имя.";
            return;
        }

        bool inserted = selected_mode == 0
            ? numbers.insert(*value) : records.insert(record{*value, name});
        message = (inserted ? "Добавлено: " : "Уже есть: ") + std::to_string(*value);
    };

    auto find_value = [&] {
        auto value = read_value();
        if (!value) return;
        if (selected_mode == 0) {
            message = (numbers.contains(*value) ? "Найдено: " : "Не найдено: ") +
                std::to_string(*value);
            return;
        }

        auto found = records.find(record{*value, {}});
        message = found
            ? "Найдено: " + std::to_string(found->get().id) + ": " + found->get().name
            : "Не найдено: " + std::to_string(*value);
    };

    auto erase_value = [&] {
        auto value = read_value();
        if (!value) return;
        bool erased = selected_mode == 0
            ? numbers.erase(*value) : records.erase(record{*value, {}});
        message = (erased ? "Удалено: " : "Не найдено: ") + std::to_string(*value);
    };

    auto generate_values = [&] {
        auto count = parse_number(count_text);
        auto minimum = parse_number(minimum_text);
        auto maximum = parse_number(maximum_text);
        auto seed = parse_number(seed_text);
        input_error = true;

        if (!count || *count < 0 || !seed || *seed < 0) {
            message = "Количество и seed: целые от 0 до " +
                std::to_string(std::numeric_limits<int>::max()) + ".";
            return;
        }
        if (!minimum || !maximum) {
            message = "Границы должны быть целыми числами от " +
                std::to_string(std::numeric_limits<int>::min()) + " до " +
                std::to_string(std::numeric_limits<int>::max()) + ".";
            return;
        }

        try {
            numbers = generate_tree(*count, *minimum, *maximum, *seed);
        } catch (const std::invalid_argument& error) {
            message = error.what();
            return;
        }
        input_error = false;
        message = "Создано новое дерево: " + std::to_string(*count) +
            ", seed: " + std::to_string(*seed);
    };

    auto app = App::Fullscreen();
    auto mode_options = MenuOption::Toggle();
    mode_options.on_change = [&] {
        message = "Готово";
        input_error = false;
    };
    auto mode = Menu(&modes, &selected_mode, mode_options);
    InputOption input_options;
    input_options.multiline = false;
    input_options.on_enter = insert_value;
    auto input = Input(&number, "Например: 7", input_options);
    auto name_input = Input(&name, "Иван Иванов", input_options);
    auto name_field = Renderer(name_input, [&] {
        return vbox({text("Имя"), name_input->Render() | border});
    });
    name_field = Maybe(name_field, [&] { return selected_mode == 1; });
    auto insert = Button("Вставить", insert_value, ButtonOption::Ascii());
    auto find = Button("Найти", find_value, ButtonOption::Ascii());
    auto erase = Button("Удалить", erase_value, ButtonOption::Ascii());
    auto order = Dropdown(&orders, &selected_order);
    InputOption generation_options;
    generation_options.multiline = false;
    generation_options.on_enter = generate_values;
    auto count_input = Input(&count_text, "10", generation_options);
    auto minimum_input = Input(&minimum_text, "0", generation_options);
    auto maximum_input = Input(&maximum_text, "99", generation_options);
    auto seed_input = Input(&seed_text, "42", generation_options);
    auto generate = Button("Новое дерево", generate_values, ButtonOption::Ascii());
    auto generation = Container::Vertical({
        count_input, minimum_input, maximum_input, seed_input, generate
    });
    generation = Renderer(generation, [&] {
        return vbox({
            separator(),
            text("Генерация чисел") | bold,
            text("Количество"), count_input->Render() | border,
            text("От (включительно)"), minimum_input->Render() | border,
            text("До (включительно)"), maximum_input->Render() | border,
            text("Seed"), seed_input->Render() | border,
            text("Заменит текущее дерево") | dim,
            generate->Render()
        });
    });
    generation = Maybe(generation, [&] { return selected_mode == 0; });
    auto exit = Button("Выход", app.ExitLoopClosure());
    mode |= CatchEvent([&](Event event) {
        if (event == Event::Tab) {
            input->TakeFocus();
            return true;
        }
        if (event == Event::TabReverse) {
            exit->TakeFocus();
            return true;
        }
        return false;
    });
    auto controls = Container::Vertical({
        mode, input, name_field, insert, find, erase, order, generation, exit
    });
    Box fields_box;
    Element fields_content;
    bool manual_scroll = false;
    int scroll_position = 0;

    auto screen = Renderer(controls, [&] {
        std::ostringstream output;
        std::ostringstream traversal;
        std::size_t active_size = 0;
        auto write_tree = [&](const auto& tree) {
            tree.print_tree(output, [](std::ostream& stream, const auto& value) {
                stream << value;
            });
            bool first = true;
            tree.traverse(order_values[selected_order], [&](const auto& value) {
                if (!first) traversal << ' ';
                traversal << value;
                first = false;
            });
            active_size = tree.size();
        };

        if (selected_mode == 0)
            write_tree(numbers);
        else
            write_tree(records);
        std::istringstream rows(output.str());
        Elements tree_lines;
        for (std::string line; std::getline(rows, line);)
            tree_lines.push_back(text(line));

        std::string result = active_size == 0 ? "(пусто)" : traversal.str();

        fields_content = vbox({
            text("Режим"),
            mode->Render(),
            separator(),
            text(selected_mode == 0 ? "Число" : "ID"),
            input->Render() | border,
            name_field->Render(),
            text("Enter — вставить") | dim,
            insert->Render(),
            find->Render(),
            erase->Render(),
            separator(),
            text("Порядок обхода"),
            order->Render(),
            generation->Render()
        });
        auto fields = fields_content;
        if (manual_scroll)
            fields = fields | focusPosition(0, scroll_position);
        fields = fields | vscroll_indicator | yframe | flex | reflect(fields_box);

        auto panel = vbox({
            text("Управление") | bold,
            separator(),
            fields,
            separator(),
            exit->Render()
        }) | size(WIDTH, EQUAL, 28) | border;

        auto content = vbox({
            text("Дерево") | bold,
            separator(),
            vbox(tree_lines) | frame | flex,
            separator(),
            text("Размер: " + std::to_string(active_size)),
            paragraph("Результат обхода: " + result)
        }) | flex | border;

        return vbox({
            text("AVL-дерево") | bold | center,
            hbox({panel, content}) | flex,
            paragraph(message) | color(input_error ? Color::Red : Color::Green),
            text("Tab — переход · Enter — действие · Esc / Ctrl+D — выход")
                | dim
        });
    });

    screen |= CatchEvent([&](Event event) {
        if (event.is_mouse() &&
            (event.mouse().button == Mouse::WheelUp ||
             event.mouse().button == Mouse::WheelDown)) {
            if (!fields_box.Contain(event.mouse().x, event.mouse().y))
                return false;

            auto requirement = fields_content->requirement();
            int height = fields_box.y_max - fields_box.y_min;
            int maximum = std::max(0, requirement.min_y - height - 1);
            int center = manual_scroll ? scroll_position :
                (requirement.focused.box.y_min + requirement.focused.box.y_max) / 2;
            int offset = std::clamp(center - height / 2, 0, maximum);
            int step = event.mouse().button == Mouse::WheelDown ? 3 : -3;
            scroll_position = std::clamp(offset + step, 0, maximum) + height / 2;
            manual_scroll = true;
            return true;
        }
        if (!event.is_mouse() || event.mouse().motion == Mouse::Pressed)
            manual_scroll = false;
        if (event != Event::Escape && event != Event::CtrlD) return false;
        app.Exit();
        return true;
    });

    app.Loop(screen);
    return 0;

}
