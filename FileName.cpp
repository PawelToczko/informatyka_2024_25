
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

// Struktura przechowuj�ca informacje o graczach
struct Player {
    std::string name;
    int score;
};
struct PlayerScore {
    std::string name;
    int score;
};
std::vector<PlayerScore> leaderboard;
// Klasa reprezentuj�ca gr�
void addToLeaderboard(const std::string& playerName, int playerScore) {
    leaderboard.push_back({ playerName, playerScore });

    // Opcjonalnie ogranicz do top 10 wynik�w
    if (leaderboard.size() > 10) {
        leaderboard.pop_back();
    }
}
void saveLeaderboardToFile(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Nie udalo sie otworzyc pliku do zapisu leaderboard");
    }

    for (const auto& entry : leaderboard) {
        file << entry.name << " " << entry.score << "\n";
    }

    file.close();
}
void loadLeaderboardFromFile(const std::string& filename) {
    leaderboard.clear();
    std::ifstream file(filename);
    if (!file.is_open()) {
        return; // Je�li plik nie istnieje, pomi�
    }

    std::string name;
    int score;
    while (file >> name >> score) {
        leaderboard.push_back({ name, score });
    }

    file.close();

    // Sortowanie po wczytaniu
    std::sort(leaderboard.begin(), leaderboard.end(), [](const PlayerScore& a, const PlayerScore& b) {
        return a.score < b.score;
        });

    // Ograniczenie do top 10
    if (leaderboard.size() > 10) {
        leaderboard.resize(10);
    }
}

void showLeaderboard(sf::RenderWindow& window, sf::Font& font) {
    window.clear(sf::Color::Black);

    sf::Text title("Punkty Graczy", font, 30);
    title.setPosition(300, 50);
    title.setFillColor(sf::Color::White);
    window.draw(title);

    int yOffset = 100;
    // Odwr�cenie iteracji po leaderboardzie, �eby najwy�szy wynik by� na g�rze
    for (size_t i = 0; i < leaderboard.size(); ++i) {
        std::string entry = std::to_string(i + 1) + ". " + leaderboard[leaderboard.size() - 1 - i].name + " - " + std::to_string(leaderboard[leaderboard.size() - 1 - i].score);
        sf::Text text(entry, font, 20);
        text.setPosition(200, yOffset);
        text.setFillColor(sf::Color::White);
        window.draw(text);
        yOffset += 30;
    }

    sf::Text back("Nacisnij ESC by wrocic do menu", font, 20);
    back.setPosition(250, 500);
    back.setFillColor(sf::Color::White);
    window.draw(back);

    window.display();

    // Wait for ESC key to return to menu
    sf::Event event;
    while (window.waitEvent(event)) {
        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
            break;
        }
    }
}


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
    std::vector<sf::Sprite> healthSprites; // Sprite'y reprezentuj�ce zdrowie
    std::vector<sf::Sprite> fallingHearts; // Sprite'y reprezentuj�ce spadaj�ce serca
    sf::Clock enemyBulletClock;
    sf::Time enemyBulletCooldown = sf::seconds(1.0f);
    sf::Texture playerShootingTexture; // Tekstura dla animacji strza�u
    sf::Clock shootingAnimationClock; // Zegar do kontrolowania czasu zmiany tekstury
    sf::Time shootingAnimationDuration = sf::seconds(0.2f); // Czas trwania animacji

    int score = 0;
    int level = 1;
    int health = 3;
    bool isHelpScreen = false;
    bool isExitConfirmation = false;
    bool gameRunning = true;

    // Flaga �ledz�ca, czy klawisz spacji zosta� zwolniony
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
            throw std::runtime_error("Nie uda�o si� wczyta� czcionki");
        }

        // Wczytywanie tekstur
        if (!playerTexture.loadFromFile("player.png") ||
            !enemyTexture.loadFromFile("enemy.png") ||
            !bulletTexture.loadFromFile("bullet.png") ||
            !enemyBulletTexture.loadFromFile("enemy_bullet.png") ||
            !backgroundTexture.loadFromFile("background.png") ||
            !healthTexture.loadFromFile("heart.png") ||
            !playerShootingTexture.loadFromFile("player_shooting.png")) {
            throw std::runtime_error("Nie uda�o si� wczyta� tekstur");
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
        const float MIN_DISTANCE = 50.0f; 
        enemies.clear();

        for (int i = 0; i < 4 + level; ++i) {
            sf::Sprite enemy(enemyTexture);
            bool validPosition = false;

            
            while (!validPosition) {
              
                float x = rand() % 700;
                float y = rand() % 200;
                enemy.setPosition(x, y);

               
                validPosition = true;
                for (const auto& existingEnemy : enemies) {
                    float dx = existingEnemy.getPosition().x - x;
                    float dy = existingEnemy.getPosition().y - y;
                    float distance = std::sqrt(dx * dx + dy * dy);

                    if (distance < MIN_DISTANCE) {
                        validPosition = false;
                        break;
                    }
                }
            }
            enemies.push_back(enemy);
        }
    }

    void initializeHealthSprites() {
        healthSprites.clear(); // Usu� poprzednie sprite'y
        for (int i = 0; i < health; ++i) {
            sf::Sprite heart(healthTexture);
            heart.setPosition(680 + i * 40, 550); // Ustawienie pozycji serc
            healthSprites.push_back(heart);
        }
    }


    void updateHealthSprites() {
        if (health < healthSprites.size()) {
            healthSprites.pop_back(); // Usuni�cie ostatniego sprite'a zdrowia
        }
    }
    void showGameOverScreen() {
        sf::Text gameOverText, scoreText, retryText, exitText;
        setupText(&gameOverText, "GAME OVER", 250, 150, 50);
        setupText(&scoreText, "Final Score: " + std::to_string(score), 250, 250, 30);
        setupText(&retryText, "Nacisnij R by sprobowac ponownie", 250, 350, 20);
        setupText(&exitText, "nacisnij ESC by wylaczyc gre", 250, 400, 20);

        while (window.isOpen()) {
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed) {
                    window.close();
                }
                if (event.type == sf::Event::KeyPressed) {
                    if (event.key.code == sf::Keyboard::R) {
                        startNewGame(); // Restart the game
                        return; // Exit the Game Over screen
                    }
                    if (event.key.code == sf::Keyboard::Escape) {
                        window.close(); // Exit the game
                    }
                }
            }

            window.clear();
            window.draw(gameOverText);
            window.draw(scoreText);
            window.draw(retryText);
            window.draw(exitText);
            window.display();
        }
    }

    void updateFallingHearts() {
        // Aktualizacja pozycji spadaj�cych serc
        for (auto& heart : fallingHearts) {
            heart.move(0, 0.05f); // Pr�dko�� spadania serca
        }

        // Usuwanie serc, kt�re spad�y poza ekran
        fallingHearts.erase(std::remove_if(fallingHearts.begin(), fallingHearts.end(), [](const sf::Sprite& heart) {
            return heart.getPosition().y > 600;
            }), fallingHearts.end());

        // Kolizja gracza z sercami
        for (auto it = fallingHearts.begin(); it != fallingHearts.end();) {
            if (it->getGlobalBounds().intersects(player.getGlobalBounds())) {
                it = fallingHearts.erase(it); // Usuni�cie serca po zebraniu
                if (health < 3) {
                    health++; // Zwi�kszenie zdrowia
                    initializeHealthSprites(); // Aktualizacja wy�wietlania serc
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
                    isHelpScreen = !isHelpScreen; // Prze��czanie ekranu pomocy
                }
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
                    saveGame();
                }

                if (isExitConfirmation) {
                    if (event.key.code == sf::Keyboard::T) {
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

                // Ustawienie tekstury strza�u
                player.setTexture(playerShootingTexture);
                shootingAnimationClock.restart(); // Restart zegara animacji
            }
        }
    }


    void updateGame() {
        if (shootingAnimationClock.getElapsedTime() > shootingAnimationDuration) {
            player.setTexture(playerTexture); // Przywr�cenie oryginalnej tekstury
        }

        // Aktualizacja pozycji pocisk�w gracza
        for (auto& bullet : bullets) {
            bullet.move(0, -0.2);
        }

        // Usuwanie pocisk�w gracza poza ekranem
        bullets.erase(std::remove_if(bullets.begin(), bullets.end(), [](const sf::Sprite& b) {
            return b.getPosition().y < 0;
            }), bullets.end());

        // Kolizje pocisk�w gracza z wrogami
        for (auto& bullet : bullets) {
            for (auto it = enemies.begin(); it != enemies.end();) {
                if (bullet.getGlobalBounds().intersects(it->getGlobalBounds())) {
                    // Drop serca z przeciwnika
                    if (rand() % 10 == 0) { // 10%
                        sf::Sprite heart(healthTexture);
                        heart.setPosition(it->getPosition().x, it->getPosition().y);
                        fallingHearts.push_back(heart);
                    }

                    it = enemies.erase(it); // Usuni�cie przeciwnika
                    score += 10;
                }
                else {
                    ++it;
                }
            }
        }

        // Generowanie pocisk�w przeciwnik�w z ograniczon� cz�stotliwo�ci�
        if (enemyBulletClock.getElapsedTime() > enemyBulletCooldown && !enemies.empty()) {
            int randomEnemy = rand() % enemies.size();
            sf::Sprite enemyBullet(enemyBulletTexture);
            enemyBullet.setPosition(enemies[randomEnemy].getPosition().x + 25, enemies[randomEnemy].getPosition().y + 50);
            enemyBullets.push_back(enemyBullet);
            enemyBulletClock.restart();
        }

        // Aktualizacja pozycji pocisk�w przeciwnik�w
        for (auto& enemyBullet : enemyBullets) {
            enemyBullet.move(0, 0.06);
        }

        // Usuwanie pocisk�w przeciwnik�w poza ekranem
        enemyBullets.erase(std::remove_if(enemyBullets.begin(), enemyBullets.end(), [](const sf::Sprite& b) {
            return b.getPosition().y > 600;
            }), enemyBullets.end());

        // Kolizje pocisk�w przeciwnik�w z graczem
        for (auto it = enemyBullets.begin(); it != enemyBullets.end();) {
            if (it->getGlobalBounds().intersects(player.getGlobalBounds())) {
                it = enemyBullets.erase(it);
                health--;
                updateHealthSprites(); // Aktualizacja sprite'�w zdrowia
                if (health <= 0) {
                    gameRunning = false;

                    // Dodanie wyniku gracza do leaderboard
                    addToLeaderboard("Gracz", score); // Zmie� "Gracz" na aktualn� nazw� gracza, je�li dodasz jej wprowadzanie

                    // Zapisanie leaderboard do pliku
                    saveLeaderboardToFile("leaderboard.txt");
                    showGameOverScreen();
                }

            }
            else {
                ++it;
            }
        }

        // Sprawdzenie, czy gracz wygra� poziom
        if (enemies.empty()) {
            level++;
            spawnEnemies();
        }

        // Aktualizacja spadaj�cych serc
        updateFallingHearts();
       

        // Aktualizacja wy�wietlanych danych
        scoreText.setString("Score: " + std::to_string(score));
    }


    void render() {
        window.clear();
        window.draw(background);

        if (isHelpScreen) {
            setupText(&helpText, "Sterowanie:\nLewo/Prawo - Strzalki\nStrzelanie - Spacja\nESC - Wyjscie\nF1 - Pomoc\nF5 - Zapisz Gr�", 150, 150, 24);
            window.draw(helpText);
        }
        else if (isExitConfirmation) {
            setupText(&exitConfirmationText, "Czy chcesz wyjsc z gry (T/N)", 200, 200, 30);
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
                window.draw(heart); // Rysowanie sprite'�w zdrowia
            }
            for (const auto& heart : fallingHearts) {
                window.draw(heart); // Rysowanie spadaj�cych serc
            }
            window.draw(scoreText);
        }

        window.display();
    }

    void saveGame() {
        std::ofstream saveFile("savegame.txt");
        if (!saveFile.is_open()) {
            throw std::runtime_error("Nie udalo sie otworzyc pliku do zapisu");
        }

        // Zapisanie podstawowych danych gry
        saveFile << score << '\n';  // Wynik
        saveFile << level << '\n';  // Poziom
        saveFile << health << '\n'; // Zdrowie

        // Zapisanie pozycji gracza
        sf::Vector2f playerPosition = player.getPosition();
        saveFile << playerPosition.x << ' ' << playerPosition.y << '\n';

        // Zapisanie pozycji wrog�w
        for (const auto& enemy : enemies) {
            sf::Vector2f enemyPosition = enemy.getPosition();
            saveFile << enemyPosition.x << ' ' << enemyPosition.y << '\n';
        }
        saveFile << "END\n"; // Znak ko�ca sekcji wrog�w

        saveFile.close();
        std::cout << "Gra zostala zapisana!" << std::endl;
    }
    void loadGame() {
        std::ifstream saveFile("savegame.txt");
        if (!saveFile.is_open()) {
            throw std::runtime_error("Nie uda�o sie otworzyc pliku do odczytu");
        }

        std::string line;
        // Wczytanie podstawowych danych gry
        if (std::getline(saveFile, line)) score = std::stoi(line);
        if (std::getline(saveFile, line)) level = std::stoi(line);
        if (std::getline(saveFile, line)) health = std::stoi(line);

        initializeHealthSprites(); // Aktualizacja sprite'�w zdrowia

        // Wczytanie pozycji gracza
        if (std::getline(saveFile, line)) {
            std::istringstream iss(line);
            float x, y;
            if (iss >> x >> y) {
                player.setPosition(x, y);
            }
        }

        // Wczytanie pozycji wrog�w
        enemies.clear();
        while (std::getline(saveFile, line)) {
            if (line == "END") break; // Detekcja ko�ca sekcji wrog�w
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
        int selection = 0; // 0: Wczytaj gr�, 1: Nowa gra, 2: Leaderboard
        sf::Text menuOptions[3];

        setupText(&menuOptions[0], "Wczytaj gre", 300, 200, 30);
        setupText(&menuOptions[1], "Nowa gra", 300, 250, 30);
        setupText(&menuOptions[2], "Punkty Graczy", 300, 300, 30);

        while (window.isOpen()) {
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed) {
                    window.close();
                    return -1; // Wyj�cie z gry
                }

                if (event.type == sf::Event::KeyPressed) {
                    if (event.key.code == sf::Keyboard::Up) {
                        selection = (selection - 1 + 3) % 3;
                    }
                    if (event.key.code == sf::Keyboard::Down) {
                        selection = (selection + 1) % 3;
                    }
                    if (event.key.code == sf::Keyboard::Enter) {
                        return selection;
                    }
                }
            }

            // Pod�wietlenie wybranej opcji
            for (int i = 0; i < 3; ++i) {
                menuOptions[i].setFillColor(selection == i ? sf::Color::Yellow : sf::Color::White);
            }

            window.clear();
            for (const auto& option : menuOptions) {
                window.draw(option);
            }
            window.display();
        }

        return -1; // Domy�lny zwrot w przypadku zamkni�cia
    }




public:
    SpaceInvaders() : window(sf::VideoMode(800, 600), "Space Invaders") {
        loadResources();
        loadLeaderboardFromFile("leaderboard.txt");
        setupText(&scoreText, "Score: 0", 10, 550);
        setupText(&helpText, "F1: Help\nESC: Exit\nSpace: Shoot\nArrow Keys: Move", 200, 200, 30);

        while (true) {
            int menuChoice = showMainMenu();
            if (menuChoice == 0) {
                loadGame(); // Wczytaj stan gry
                break; // Przejd� do gry
            }
            else if (menuChoice == 1) {
                score = 0;
                level = 1;
                health = 3;
                player.setPosition(400, 500);
                spawnEnemies();
                initializeHealthSprites();
                break; // Przejd� do gry
            }
            else if (menuChoice == 2) {
                showLeaderboard(window, font); // Wy�wietl leaderboard
            }
            else {
                window.close(); // Zamkni�cie gry w przypadku wyboru -1
                break;
            }
        }
    }


    void run() {
        while (window.isOpen()) {
            handleEvents();
            if (gameRunning) {
                if (!isHelpScreen && !isExitConfirmation) {
                    updateGame();
                }
                render();
            }
            else {
                int choice = showMainMenu(); // Wyb�r nowej gry lub wczytania
                if (choice == 1) {
                    startNewGame(); // Rozpocznij now� gr�
                }
                else if (choice == 0) {
                    loadGame(); // Wczytaj zapis gry
                }
                else {
                    window.close(); // Zamknij gr�
                }
            }
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


