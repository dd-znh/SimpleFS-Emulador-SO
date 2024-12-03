#ifndef GRAPHIC_INTERFACE_H
#define GRAPHIC_INTERFACE_H

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <iostream>  // Necessário para std::cerr
#include <string>
#include <vector>

#include "disk.h"
#include "fs.h"

namespace GRAPHIC_INTERFACE {

class GraphicApp {
   private:
    sf::RenderWindow window;
    sf::Font font;

    // Botões da esquerda
    std::vector<sf::RectangleShape> buttons;
    std::vector<sf::Text> buttonTexts;
    std::vector<std::string> buttonLabels;

    // Texto central para exibir resultado
    sf::Text outputLabel;
    sf::Text resultText;
    std::string resultString;

    // Teclado numérico
    std::vector<sf::RectangleShape> numKeys;
    std::vector<sf::Text> numKeyTexts;
    std::vector<std::string> keyLabels;

    // Entrada de texto
    sf::Text inputText;
    std::string currentInput;

    // Sistema de arquivos
    INE5412_FS *fs;
    char *file;

    void createButtons();
    void createKeyboard();

    // Métodos para ações dos botões
    void handleButtonAction(const std::string &label);

   public:
    GraphicApp(INE5412_FS *fs, char *filename);
    void run();
};

}  // namespace GRAPHIC_INTERFACE

#endif
