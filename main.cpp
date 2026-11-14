#include "avl_tree.hpp"

#include <ftxui/component/app.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include <array>
#include <charconv>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include <unistd.h>

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

    avl_tree<int> tree;
    std::string number;
    std::string message = "Готово";
    bool input_error = false;
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
        message = (tree.insert(*value) ? "Добавлено: " : "Уже есть: ") +
            std::to_string(*value);
    };

    auto find_value = [&] {
        auto value = read_value();
        if (!value) return;
        message = (tree.contains(*value) ? "Найдено: " : "Не найдено: ") +
            std::to_string(*value);
    };

    auto erase_value = [&] {
        auto value = read_value();
        if (!value) return;
        message = (tree.erase(*value) ? "Удалено: " : "Не найдено: ") +
            std::to_string(*value);
    };

    auto app = App::Fullscreen();
    InputOption input_options;
    input_options.multiline = false;
    input_options.on_enter = insert_value;
    auto input = Input(&number, "Например: 7", input_options);
    auto insert = Button("Вставить", insert_value, ButtonOption::Ascii());
    auto find = Button("Найти", find_value, ButtonOption::Ascii());
    auto erase = Button("Удалить", erase_value, ButtonOption::Ascii());
    auto order = Dropdown(&orders, &selected_order);
    auto exit = Button("Выход", app.ExitLoopClosure());
    auto controls = Container::Vertical({input, insert, find, erase, order, exit});

    auto screen = Renderer(controls, [&] {
        std::ostringstream output;
        tree.print_tree(output, [](std::ostream& stream, const int& value) {
            stream << value;
        });
        std::istringstream rows(output.str());
        Elements tree_lines;
        for (std::string line; std::getline(rows, line);)
            tree_lines.push_back(text(line));

        std::ostringstream traversal;
        bool first = true;
        tree.traverse(order_values[selected_order], [&](const int& value) {
            if (!first) traversal << ' ';
            traversal << value;
            first = false;
        });
        std::string result = tree.empty() ? "(пусто)" : traversal.str();

        auto panel = vbox({
            text("Управление") | bold,
            separator(),
            text("Число"),
            input->Render() | border,
            text("Enter — вставить") | dim,
            insert->Render(),
            find->Render(),
            erase->Render(),
            separator(),
            text("Порядок обхода"),
            order->Render(),
            filler(),
            exit->Render()
        }) | size(WIDTH, EQUAL, 28) | border;

        auto content = vbox({
            text("Дерево") | bold,
            separator(),
            vbox(tree_lines) | frame | flex,
            separator(),
            text("Размер: " + std::to_string(tree.size())),
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
        if (event != Event::Escape && event != Event::CtrlD) return false;
        app.Exit();
        return true;
    });

    app.Loop(screen);
    return 0;

}
