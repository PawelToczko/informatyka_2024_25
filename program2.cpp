#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <fstream> 
#include <stdexcept>

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
    sf::Texture playerTexture, enemyTexture, bulletTexture, enemyBulletTexture, backgroundTexture, healthTexture;
    sf::Sprite player, background;
    std::vector<sf::Sprite> enemies;
    std::vector<sf::Sprite> bullets;
    std::vector<sf::Sprite> enemyBullets;
    std::vector<sf::Sprite> healthSprites; // Sprite'y reprezentujące zdrowie
    std::vector<sf::Sprite> fallingHearts; // Sprite'y reprezentujące spadające serca
    sf::Clock enemyBulletClock;
    sf::Time enemyBulletCooldown = sf::seconds(1.0f);
    sf::Texture playerShootingTexture; // Tekstura dla animacji strzału
    sf::Clock shootingAnimationClock; // Zegar do kontrolowania czasu zmiany tekstury
    sf::Time shootingAnimationDuration = sf::seconds(0.2f); // Czas trwania animacji

    int score = 0;
    int level = 1;
    int health = 3;
    bool isHelpScreen = false;
    bool isExitConfirmation = false;
    bool gameRunning = true;

    // Flaga śledząca, czy klawisz spacji został zwolniony
    bool spaceReleased = true;

    void startNewGame() {
        score = 0;
        level = 1;
        health = 3;
        spawnEnemies();
        initializeHealthSprites();
        gameRunning = true;
    }


    void loadResources() {
        // Wczytywanie czcionki
        if (!font.loadFromFile("arial.ttf")) {
            throw std::runtime_error("Nie udało się wczytać czcionki");
        }

        // Wczytywanie tekstur
        if (!playerTexture.loadFromFile("player.png") ||
            !enemyTexture.loadFromFile("enemy.png") ||
            !bulletTexture.loadFromFile("bullet.png") ||
            !enemyBulletTexture.loadFromFile("enemy_bullet.png") ||
            !backgroundTexture.loadFromFile("background.png") ||
            !healthTexture.loadFromFile("heart.png") ||
            !playerShootingTexture.loadFromFile("player_shooting.png")) {
            throw std::runtime_error("Nie udało się wczytać tekstur");
        }
        
        player.setTexture(playerTexture);
        background.setTexture(backgroundTexture);
        player.setPosition(400, 500);
    }

    void setupText(sf::Text* text, const std::string& content, float x, float y, int size = 20) {
        if (text) {
            text->setFont(font);
            text->setString(content);
            text->setCharacterSize(size);
            text->setFillColor(sf::Color::White);
            text->setPosition(x, y);
        }
    }

    void spawnEnemies() {
        enemies.clear();
        for (int i = 0; i < 5 + level; ++i) {
            sf::Sprite enemy(enemyTexture);
            enemy.setPosition(rand() % 700, rand() % 200);
            enemies.push_back(enemy);
        }
    }

    void initializeHealthSprites() {
        healthSprites.clear();
        for (int i = 0; i < health; ++i) {
            sf::Sprite heart(healthTexture);
            heart.setPosition(680 + i * 40, 550); // Pozycjonowanie sprite'ów zdrowia
            healthSprites.push_back(heart);
        }
    }

    void updateHealthSprites() {
        if (health < healthSprites.size()) {
            healthSprites.pop_back(); // Usunięcie ostatniego sprite'a zdrowia
        }
    }

    void updateFallingHearts() {
        // Aktualizacja pozycji spadających serc
        for (auto& heart : fallingHearts) {
            heart.move(0, 0.05f); // Prędkość spadania serca
        }

        // Usuwanie serc, które spadły poza ekran
        fallingHearts.erase(std::remove_if(fallingHearts.begin(), fallingHearts.end(), [](const sf::Sprite& heart) {
            return heart.getPosition().y > 600;
            }), fallingHearts.end());

        // Kolizja gracza z sercami
        for (auto it = fallingHearts.begin(); it != fallingHearts.end();) {
            if (it->getGlobalBounds().intersects(player.getGlobalBounds())) {
                it = fallingHearts.erase(it); // Usunięcie serca po zebraniu
                if (health < 3) {
                    health++; // Zwiększenie zdrowia
                    initializeHealthSprites(); // Aktualizacja wyświetlania serc
                }
            }
            else {
                ++it;
            }
        }
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
                    isHelpScreen = !isHelpScreen; // Przełączanie ekranu pomocy
                }

                if (isExitConfirmation) {
                    if (event.key.code == sf::Keyboard::Y) {
                        window.close();
                    }
                    else if (event.key.code == sf::Keyboard::N) {
                        isExitConfirmation = false;
                    }
                }
                else if (!isHelpScreen && !isExitConfirmation) {
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
                    spaceReleased = true; // Ustawienie flagi na true po zwolnieniu klawisza spacji
                }
            }
        }

        // Strzelanie pociskami gracza
        if (!isHelpScreen && !isExitConfirmation) {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space) && spaceReleased) {
                sf::Sprite bullet(bulletTexture);
                bullet.setPosition(player.getPosition().x + 25, player.getPosition().y);
                bullets.push_back(bullet);
                spaceReleased = false;

                // Ustawienie tekstury strzału
                player.setTexture(playerShootingTexture);
                shootingAnimationClock.restart(); // Restart zegara animacji
            }
        }
    }


    void updateGame() {
        if (shootingAnimationClock.getElapsedTime() > shootingAnimationDuration) {
            player.setTexture(playerTexture); // Przywrócenie oryginalnej tekstury
        }

        // Aktualizacja pozycji pocisków gracza
        for (auto& bullet : bullets) {
            bullet.move(0, -0.2);
        }

        // Usuwanie pocisków gracza poza ekranem
        bullets.erase(std::remove_if(bullets.begin(), bullets.end(), [](const sf::Sprite& b) {
            return b.getPosition().y < 0;
            }), bullets.end());

        // Kolizje pocisków gracza z wrogami
        for (auto& bullet : bullets) {
            for (auto it = enemies.begin(); it != enemies.end();) {
                if (bullet.getGlobalBounds().intersects(it->getGlobalBounds())) {
                    // Drop serca z przeciwnika
                    if (rand() % 10 == 0) { // 10%
                        sf::Sprite heart(healthTexture);
                        heart.setPosition(it->getPosition().x, it->getPosition().y);
                        fallingHearts.push_back(heart);
                    }

                    it = enemies.erase(it); // Usunięcie przeciwnika
                    score += 10;
                }
                else {
                    ++it;
                }
            }
        }

        // Generowanie pocisków przeciwników z ograniczoną częstotliwością
        if (enemyBulletClock.getElapsedTime() > enemyBulletCooldown && !enemies.empty()) {
            int randomEnemy = rand() % enemies.size();
            sf::Sprite enemyBullet(enemyBulletTexture);
            enemyBullet.setPosition(enemies[randomEnemy].getPosition().x + 25, enemies[randomEnemy].getPosition().y + 50);
            enemyBullets.push_back(enemyBullet);
            enemyBulletClock.restart();
        }

        // Aktualizacja pozycji pocisków przeciwników
        for (auto& enemyBullet : enemyBullets) {
            enemyBullet.move(0, 0.06);
        }

        // Usuwanie pocisków przeciwników poza ekranem
        enemyBullets.erase(std::remove_if(enemyBullets.begin(), enemyBullets.end(), [](const sf::Sprite& b) {
            return b.getPosition().y > 600;
            }), enemyBullets.end());

        // Kolizje pocisków przeciwników z graczem
        for (auto it = enemyBullets.begin(); it != enemyBullets.end();) {
            if (it->getGlobalBounds().intersects(player.getGlobalBounds())) {
                it = enemyBullets.erase(it);
                health--;
                updateHealthSprites(); // Aktualizacja sprite'ów zdrowia
                if (health <= 0) {
                    gameRunning = false;
                }
            }
            else {
                ++it;
            }
        }

        // Sprawdzenie, czy gracz wygrał poziom
        if (enemies.empty()) {
            level++;
            spawnEnemies();
        }

        // Aktualizacja spadających serc
        updateFallingHearts();

        // Aktualizacja wyświetlanych danych
        scoreText.setString("Score: " + std::to_string(score));
    }

    void render() {
        window.clear();
        window.draw(background);

        if (isHelpScreen) {
            // Ekran pomocy
            setupText(&helpText, "Sterowanie:\nLewo/Prawo - Strzalki\nStrzelanie - Spacja\nESC - Wyjscie\nF1 - Pomoc", 150, 150, 24);
            window.draw(helpText);
        }
        else if (isExitConfirmation) {
            setupText(&exitConfirmationText, "Do you want to exit? (Y/N)", 200, 200, 30);
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
            for (const auto& enemyBullet : enemyBullets) {
                window.draw(enemyBullet);
            }
            for (const auto& heart : healthSprites) {
                window.draw(heart); // Rysowanie sprite'ów zdrowia
            }
            for (const auto& heart : fallingHearts) {
                window.draw(heart); // Rysowanie spadających serc
            }
            window.draw(scoreText);
        }

        window.display();
    }
    void saveGame() {
        std::ofstream saveFile("savegame.txt");
        if (saveFile.is_open()) {
            saveFile << score << '\n';
            saveFile << level << '\n';
            saveFile << health << '\n';
            saveFile << player.getPosition().x << ' ' << player.getPosition().y << '\n';

            for (const auto& enemy : enemies) {
                saveFile << enemy.getPosition().x << ' ' << enemy.getPosition().y << '\n';
            }
            saveFile << "END\n"; // Oznaczenie końca wrogów
            saveFile.close();
        }
    }
    void loadGame() {
        std::ifstream saveFile("savegame.txt");
        if (!saveFile.is_open()) {
            throw std::runtime_error("Nie udało się otworzyć pliku do odczytu");
        }

        std::string line;
        // Wczytanie podstawowych danych gry
        if (std::getline(saveFile, line)) score = std::stoi(line);
        if (std::getline(saveFile, line)) level = std::stoi(line);
        if (std::getline(saveFile, line)) health = std::stoi(line);

        initializeHealthSprites(); // Aktualizacja zdrowia

        // Wczytanie pozycji gracza
        if (std::getline(saveFile, line)) {
            std::istringstream iss(line);
            float x, y;
            if (iss >> x >> y) {
                player.setPosition(x, y);
            }
        }

        // Wczytanie pozycji wrogów
        enemies.clear();
        while (std::getline(saveFile, line)) {
            if (line == "END") break; // Detekcja końca sekcji wrogów
            std::istringstream iss(line);
            float x, y;
            if (iss >> x >> y) {
                sf::Sprite enemy(enemyTexture);
                enemy.setPosition(x, y);
                enemies.push_back(enemy);
            }
        }

        saveFile.close();
    }
    int showMainMenu() {
        int selection = 0; // 0: Wczytaj grę, 1: Nowa gra
        sf::Text menuOptions[2];

        setupText(&menuOptions[0], "Wczytaj gre", 300, 200, 30);
        setupText(&menuOptions[1], "Nowa gra", 300, 250, 30);

        while (window.isOpen()) {
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed) {
                    window.close();
                    return -1; // Wyjście z gry
                }

                if (event.type == sf::Event::KeyPressed) {
                    if (event.key.code == sf::Keyboard::Up) {
                        selection = (selection - 1 + 2) % 2;
                    }
                    if (event.key.code == sf::Keyboard::Down) {
                        selection = (selection + 1) % 2;
                    }
                    if (event.key.code == sf::Keyboard::Enter) {
                        return selection;
                    }
                }
            }

            // Podświetlenie wybranej opcji
            menuOptions[0].setFillColor(selection == 0 ? sf::Color::Yellow : sf::Color::White);
            menuOptions[1].setFillColor(selection == 1 ? sf::Color::Yellow : sf::Color::White);

            window.clear();
            window.draw(menuOptions[0]);
            window.draw(menuOptions[1]);
            window.display();
        }

        return -1; // Domyślny zwrot w przypadku zamknięcia
    }




public:
    SpaceInvaders() : window(sf::VideoMode(800, 600), "Space Invaders") {
        loadResources();
        setupText(&scoreText, "Score: 0", 10, 550);
        setupText(&helpText, "F1: Help\nESC: Exit\nSpace: Shoot\nArrow Keys: Move", 200, 200, 30);

        int menuChoice = showMainMenu();
        if (menuChoice == 0) {
            loadGame(); // Wczytaj stan gry
        }
        else if (menuChoice == 1) {
            score = 0;
            level = 1;
            health = 3;
            player.setPosition(400, 500);
            spawnEnemies();
            initializeHealthSprites();
        }
        else {
            window.close(); // Zamknięcie gry w przypadku wyboru -1
        }
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

