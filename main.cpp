#include "avl_tree.hpp"

#include <ftxui/component/app.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include <sstream>
#include <string>
#include <vector>

int main() {
    using namespace ftxui;

    avl_tree<int> tree;
    std::string number;
    std::vector<std::string> orders = {"КЛП", "КПЛ", "ЛПК", "ЛКП", "ПЛК", "ПКЛ"};
    int selected_order = 3;

    auto app = App::Fullscreen();
    auto input = Input(&number, "Например: 7");
    auto order = Dropdown(&orders, &selected_order);
    auto exit = Button("Выход", app.ExitLoopClosure());
    auto controls = Container::Vertical({input, order, exit});

    auto screen = Renderer(controls, [&] {
        std::ostringstream output;
        tree.print_tree(output, [](std::ostream& stream, const int& value) {
            stream << value;
        });
        std::istringstream rows(output.str());
        Elements tree_lines;
        for (std::string line; std::getline(rows, line);)
            tree_lines.push_back(text(line));

        auto panel = vbox({
            text("Управление") | bold,
            separator(),
            text("Число"),
            input->Render() | border,
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
            text("Результат обхода: —")
        }) | flex | border;

        return vbox({
            text("AVL-дерево") | bold | center,
            hbox({panel, content}) | flex,
            text("Готово"),
            text("Tab — переход · Стрелки — выбор · Enter — подтвердить · Esc — выход")
                | dim
        });
    });

    screen |= CatchEvent([&](Event event) {
        if (event != Event::Escape) return false;
        app.Exit();
        return true;
    });

    app.Loop(screen);
    return 0;

}
