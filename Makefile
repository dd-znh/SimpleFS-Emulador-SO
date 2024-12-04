# Variáveis
CXX = g++
CXXFLAGS = -Wall -g
LDFLAGS = -lsfml-graphics -lsfml-window -lsfml-system

# Nomes dos executáveis
TARGET_SHELL = simplefs
TARGET_GUI = simplefs_gui

# Objetos necessários
OBJECTS_SHELL = shell.o fs.o disk.o
OBJECTS_GUI = gui.o fs.o disk.o fileops.o

# Regras principais
.PHONY: all gui clean

all: $(TARGET_SHELL)

gui: $(TARGET_GUI)

# Alvo para o shell
$(TARGET_SHELL): $(OBJECTS_SHELL)
	$(CXX) $(CXXFLAGS) $^ -o $@

# Alvo para o GUI
$(TARGET_GUI): $(OBJECTS_GUI)
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

# Regra genérica para compilação de objetos
%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Limpeza
clean:
	rm -f *.o $(TARGET_SHELL) $(TARGET_GUI)
