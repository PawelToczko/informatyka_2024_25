#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>

// Struktura przechowująca informacje o graczach
struct Player {
    std::string name;
    int score;
};

// Klasa reprezentująca grę
class SpaceInvaders {
private:
    sf::RenderWindow window;
    sf::Font font;
    sf::Text scoreText, helpText, exitConfirmationText;
    sf::Texture playerTexture, enemyTexture, bulletTexture, backgroundTexture;
    sf::Sprite player, background;
    std::vector<sf::Sprite> enemies;
    std::vector<sf::Sprite> bullets;

    int score = 0;
    int level = 1;
    bool isHelpScreen = false;
    bool isExitConfirmation = false;
    bool gameRunning = true;

    // Flaga śledząca, czy klawisz spacji został zwolniony
    bool spaceReleased = true;

    // Dane do zapisu gry
    std::vector<Player> players;

    void loadResources() {
        // Wczytywanie czcionki
        if (!font.loadFromFile("arial.ttf")) {
            throw std::runtime_error("Nie udało się wczytać czcionki");
        }

        // Wczytywanie tekstur
        if (!playerTexture.loadFromFile("player.png") ||
            !enemyTexture.loadFromFile("enemy.png") ||
            !bulletTexture.loadFromFile("bullet.png") ||
            !backgroundTexture.loadFromFile("background.png")) {
            throw std::runtime_error("Nie udało się wczytać tekstur");
        }

        player.setTexture(playerTexture);
        background.setTexture(backgroundTexture);
        player.setPosition(400, 500);

        // Skalowanie dużych tekstur
        enemyTexture.setSmooth(true);
        bulletTexture.setSmooth(true);
        for (auto& texture : { &enemyTexture, &bulletTexture }) {
            sf::Vector2u size = texture->getSize();
            sf::Sprite sprite;
            sprite.setTexture(*texture);
            sprite.setScale(50.0f / size.x, 50.0f / size.y); // Ustawianie skali do 50x50 pikseli
        }
    }

    void setupText(sf::Text& text, const std::string& content, float x, float y, int size = 20) {
        text.setFont(font);
        text.setString(content);
        text.setCharacterSize(size);
        text.setFillColor(sf::Color::White);
        text.setPosition(x, y);
    }

    void spawnEnemies() {
        enemies.clear();
        for (int i = 0; i < 5 + level; ++i) {
            sf::Sprite enemy(enemyTexture);
            enemy.setPosition(rand() % 700, rand() % 200);
            enemies.push_back(enemy);
        }
    }

    void saveGameState() {
        std::ofstream outFile("gamestate.txt");
        if (!outFile) return;

        outFile << score << " " << level << "\n";
        for (const auto& enemy : enemies) {
            outFile << enemy.getPosition().x << " " << enemy.getPosition().y << "\n";
        }
        outFile.close();
    }

    void loadGameState() {
        std::ifstream inFile("gamestate.txt");
        if (!inFile) return;

        inFile >> score >> level;
        enemies.clear();
        float x, y;
        while (inFile >> x >> y) {
            sf::Sprite enemy(enemyTexture);
            enemy.setPosition(x, y);
            enemies.push_back(enemy);
        }
        inFile.close();
    }

    void handleEvents() {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Escape) {
                    isExitConfirmation = true;
                }

                if (event.key.code == sf::Keyboard::F1) {
                    isHelpScreen = !isHelpScreen;
                }

                if (!isHelpScreen && !isExitConfirmation) {
                    if (event.key.code == sf::Keyboard::Left && player.getPosition().x > 0) {
                        player.move(-10, 0);
                    }
                    else if (event.key.code == sf::Keyboard::Right && player.getPosition().x < 760) {
                        player.move(10, 0);
                    }
                }
            }

            if (event.type == sf::Event::KeyReleased) {
                if (event.key.code == sf::Keyboard::Space) {
                    spaceReleased = true;
                }
            }
        }

        // Strzelanie pociskami
        if (!isHelpScreen && !isExitConfirmation) {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space) && spaceReleased) {
                sf::Sprite bullet(bulletTexture);
                bullet.setPosition(player.getPosition().x + 25, player.getPosition().y);
                bullets.push_back(bullet);
                spaceReleased = false; // Ustawienie flagi na fałsz po stworzeniu pocisku
            }
        }
    }

    void updateGame() {
        // Aktualizacja pozycji pocisków
        for (auto& bullet : bullets) {
            bullet.move(0, -0.2);
        }

        // Usuwanie pocisków poza ekranem
        bullets.erase(std::remove_if(bullets.begin(), bullets.end(), [](const sf::Sprite& b) {
            return b.getPosition().y < 0;
            }), bullets.end());

        // Kolizje pocisków z wrogami
        for (auto& bullet : bullets) {
            for (auto it = enemies.begin(); it != enemies.end();) {
                if (bullet.getGlobalBounds().intersects(it->getGlobalBounds())) {
                    it = enemies.erase(it);
                    score += 10;
                }
                else {
                    ++it;
                }
            }
        }

        // Sprawdzenie, czy gracz wygrał poziom
        if (enemies.empty()) {
            level++;
            spawnEnemies();
        }

        // Aktualizacja wyświetlanych danych
        scoreText.setString("Score: " + std::to_string(score));
    }

    void render() {
        window.clear();
        window.draw(background);

        if (isHelpScreen) {
            window.draw(helpText);
        }
        else if (isExitConfirmation) {
            window.draw(exitConfirmationText);
        }
        else {
            window.draw(player);
            for (const auto& enemy : enemies) {
                window.draw(enemy);
            }
            for (const auto& bullet : bullets) {
                window.draw(bullet);
            }
            window.draw(scoreText);
        }

        window.display();
    }

public:
    SpaceInvaders() : window(sf::VideoMode(800, 600), "Space Invaders") {
        loadResources();
        setupText(scoreText, "Score: 0", 10, 10);
        setupText(helpText, "F1: Help\nESC: Exit\nSpace: Shoot\nArrow Keys: Move", 200, 200, 30);
        setupText(exitConfirmationText, "Do you want to exit? (Y/N)", 200, 200, 30);
        spawnEnemies();
    }

    void run() {
        while (window.isOpen() && gameRunning) {
            handleEvents();
            if (!isHelpScreen && !isExitConfirmation) {
                updateGame();
            }
            render();
        }
    }
};

int main() {
    try {
        SpaceInvaders game;
        game.run();
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
