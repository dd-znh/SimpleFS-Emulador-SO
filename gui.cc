#include "gui.h"

#include "disk.h"
#include "fileops.h"

namespace GRAPHIC_INTERFACE {

GraphicApp::GraphicApp(INE5412_FS *fs, char *filename) : window(sf::VideoMode(900, 400), "SympleFS"),
                                                         buttonLabels({"format", "mount", "debug", "create", "delete", "cat", "copyin", "copyout"}),
                                                         keyLabels({"7", "8", "9", "4", "5", "6", "1", "2", "3", "0", "<"}) {
    window.setFramerateLimit(60);

    if (!font.loadFromFile("/System/Library/Fonts/Supplemental/Arial.ttf")) {
        throw std::runtime_error("Failed to load font!");
    }

    this->fs = fs;
    this->file = filename;

    createButtons();
    createKeyboard();

    outputLabel.setFont(font);
    outputLabel.setCharacterSize(16);
    outputLabel.setFillColor(sf::Color::Black);
    outputLabel.setPosition(190, 20);
    outputLabel.setString("Output:");

    // Inicializar texto central
    resultText.setFont(font);
    resultText.setCharacterSize(14);
    resultText.setFillColor(sf::Color::Black);
    resultText.setPosition(190, 40);
    resultText.setString("");

    // Inicializar entrada de texto
    inputText.setFont(font);
    inputText.setCharacterSize(24);
    inputText.setFillColor(sf::Color::Black);
    inputText.setPosition(640, 20);
    inputText.setString("Inode: ");
}

void GraphicApp::createButtons() {
    float buttonHeight = 380.f / buttonLabels.size();
    for (size_t i = 0; i < buttonLabels.size(); ++i) {
        sf::RectangleShape button(sf::Vector2f(160, buttonHeight - 5));
        button.setPosition(10, 10 + i * buttonHeight);
        button.setFillColor(sf::Color::Blue);
        buttons.push_back(button);

        sf::Text buttonText;
        buttonText.setFont(font);
        buttonText.setString(buttonLabels[i]);
        buttonText.setCharacterSize(20);
        buttonText.setFillColor(sf::Color::White);
        buttonText.setPosition(30, 10 + i * buttonHeight + 10);
        buttonTexts.push_back(buttonText);
    }
}

void GraphicApp::createKeyboard() {
    int rows = 4, cols = 3;
    float keyWidth = 70.f, keyHeight = 70.f;

    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            if (i * cols + j >= keyLabels.size()) break;

            sf::RectangleShape key(sf::Vector2f(keyWidth, keyHeight));
            key.setPosition(640 + j * (keyWidth + 10), 60 + i * (keyHeight + 10));
            key.setFillColor(sf::Color::Blue);
            numKeys.push_back(key);

            sf::Text keyText;
            keyText.setFont(font);
            keyText.setString(keyLabels[i * cols + j]);
            keyText.setCharacterSize(20);
            keyText.setFillColor(sf::Color::White);
            keyText.setPosition(650 + j * (keyWidth + 10), 70 + i * (keyHeight + 10));
            numKeyTexts.push_back(keyText);
        }
    }
}

void GraphicApp::handleButtonAction(const std::string &label) {
    if (label == "format") {
        if (fs->fs_format()) {
            resultString.append("disk formatted.\n");
        } else {
            resultString.append("format failed!\n");
        }
    } else if (label == "mount") {
        if (fs->fs_mount()) {
            resultString.append("disk mounted.\n");
        } else {
            resultString.append("mount failed!\n");
        }
    } else if (label == "debug") {
        resultString.append(fs->fs_debug().str());
    } else if (label == "create") {
        int inumber = fs->fs_create();

        if (inumber > 0) {
            resultString.append("created inode " + std::to_string(inumber) + "\n");
        } else {
            resultString.append("create failed.\n");
        }
    } else if (label == "delete") {
        if (!currentInput.empty()) {
            int inumber = std::stoi(currentInput);
            int result = fs->fs_delete(inumber);

            if (result) {
                resultString.append("inode " + std::to_string(inumber) + " deleted\n");
            } else {
                resultString.append("delete failed.\n");
            }
        } else {
            resultString.append("no inode selected.\n");
        }
    } else if (label == "cat") {
        if (currentInput == "") {
            resultString.append("no inode selected.\n");
        } else {
            resultString.append(File_Ops::cat(std::stoi(currentInput), fs).str());
        }
    } else if (label == "copyin") {
        if (currentInput == "") {
            resultString.append("no inode selected.\n");
        } else if (File_Ops::do_copyin(file, std::stoi(currentInput), fs)) {
            std::string f = file;
            resultString.append("copied file " + f + " to inode " + currentInput + "\n");
        } else {
            resultString.append("copy failed\n");
        }
    } else if (label == "copyout") {
        if (currentInput == "") {
            resultString.append("no inode selected.\n");
        } else if (File_Ops::do_copyout(std::stoi(currentInput), file, fs)) {
            std::string f = file;
            resultString.append("copied inode " + currentInput + " to file " + f + "\n");
        } else {
            resultString.append("copy failed\n");
        }
    }

    resultString += "\n################\n";

    resultText.setString(resultString);
}

void GraphicApp::run() {
    float scrollSpeed = 15.0f;                  // Velocidade de rolagem
    sf::FloatRect textArea(190, 40, 600, 300);  // Definir a área da janela onde o texto central será exibido
    float textPositionY = 40.f;                 // Posição inicial Y do texto

    // View para rolar o texto
    sf::View textView = window.getView();  // Usaremos o mesmo view da janela, mas ajustado

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }

            // Detectar cliques
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                sf::Vector2f mousePos(event.mouseButton.x, event.mouseButton.y);

                // Executar ação de cada botão, similar a shell
                for (size_t i = 0; i < buttons.size(); ++i) {
                    if (buttons[i].getGlobalBounds().contains(mousePos)) {
                        handleButtonAction(buttonLabels[i]);
                    }
                }

                // Dedicado ao teclado numérico
                for (size_t i = 0; i < numKeys.size(); ++i) {
                    if (numKeys[i].getGlobalBounds().contains(mousePos)) {
                        if (keyLabels[i] == "<") {
                            if (!currentInput.empty()) {
                                currentInput.pop_back();
                            }
                        } else {
                            currentInput += keyLabels[i];
                        }
                        inputText.setString("Inode: " + currentInput);
                    }
                }
            }

            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Up) {
                    textPositionY -= scrollSpeed;
                } else if (event.key.code == sf::Keyboard::Down) {
                    textPositionY += scrollSpeed;
                }
                resultText.setPosition(190, textPositionY);
            }
        }

        window.clear(sf::Color::White);

        // Desenhar os botões da esquerda
        for (size_t i = 0; i < buttons.size(); ++i) {
            window.draw(buttons[i]);
            window.draw(buttonTexts[i]);
        }

        // Desenhar a área de resultado (com rolagem aplicada)
        // window.draw(outputLabel);
        window.draw(resultText);

        // Desenhar o teclado numérico
        for (size_t i = 0; i < numKeys.size(); ++i) {
            window.draw(numKeys[i]);
            window.draw(numKeyTexts[i]);
        }

        // Desenhar a entrada de texto
        window.draw(inputText);

        window.display();
    }
}

}  // namespace GRAPHIC_INTERFACE

int main(int argc, char *argv[]) {
    if (argc != 4) {
        cout << "use: " << argv[0] << " <diskfile> <nblocks> <io_file>\n";
        return 1;
    }

    try {
        Disk disk(argv[1], atoi(argv[2]));
        INE5412_FS fs(&disk);

        GRAPHIC_INTERFACE::GraphicApp app(&fs, argv[3]);
        app.run();
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
    return 0;
}