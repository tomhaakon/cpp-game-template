#pragma once
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <unordered_set>
#include <vector>

struct HerdChallengeConfig {
    bool enabled = true;
    float standstillSeconds = 2.0f;
    int startingSlimes = 1;
    int slimesPerLevel = 1;
    float nextRoundDelay = 1.0f;
    bool oneHitChallenge = true;
    float resultPopupDelay = 2.0f;
};
enum class HerdChallengeState { Herding, Swinging, LevelComplete, Missed };
class HerdChallenge {
  public:
    bool load(const std::filesystem::path &, std::string &error);
    bool save(const std::filesystem::path &, std::string &error) const;
    bool apply(const HerdChallengeConfig &, std::string &error);
    void reset();
    bool updateBeforeCombat(float deltaTime, bool playerMoved, bool playerAttacking,
                            const std::function<bool()> &triggerAttack, int aliveMonsters);
    void updateAfterCombat(float deltaTime, bool playerAttacking,
                           const std::vector<std::uint64_t> &hitIds);
    void beginRound(int monsterCount);
    void updateResultDelay(float deltaTime);
    [[nodiscard]] bool resultReady() const { return resultDelayRemaining_ <= 0.0f; }
    bool consumeSpawnRequest();
    bool nextLevel();
    bool retryLevel();
    [[nodiscard]] int requestedMonsterCount() const;
    [[nodiscard]] const HerdChallengeConfig &config() const { return config_; }
    [[nodiscard]] int level() const { return level_; }
    [[nodiscard]] float stillTime() const { return stillTime_; }
    [[nodiscard]] HerdChallengeState state() const { return state_; }
    [[nodiscard]] std::size_t hitCount() const { return hitIds_.size(); }
    [[nodiscard]] int requiredHits() const { return requiredHits_; }
  private:
    HerdChallengeConfig config_{};
    HerdChallengeState state_ = HerdChallengeState::Herding;
    int level_ = 0, requiredHits_ = 0;
    float stillTime_ = 0.0f;
    float resultDelayRemaining_ = 0.0f;
    bool spawnRequested_ = true, observedAttack_ = false;
    std::unordered_set<std::uint64_t> hitIds_;
};
const char *herdChallengeStateName(HerdChallengeState state);
