#include "gameplay/HerdChallenge.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <functional>
#include <nlohmann/json.hpp>
#include <vector>
namespace { using json = nlohmann::ordered_json; }
bool HerdChallenge::apply(const HerdChallengeConfig &config, std::string &error) {
    if (!std::isfinite(config.standstillSeconds) || config.standstillSeconds <= 0 ||
        config.startingSlimes < 1 || config.slimesPerLevel < 0 ||
        !std::isfinite(config.nextRoundDelay) || config.nextRoundDelay < 0 ||
        !std::isfinite(config.resultPopupDelay) || config.resultPopupDelay < 0) {
        error = "Herd Challenge settings are invalid"; return false;
    }
    config_ = config; reset(); return true;
}
bool HerdChallenge::load(const std::filesystem::path &path, std::string &error) {
    try {
        std::ifstream input(path);
        if (!input) { reset(); return true; }
        const auto root = json::parse(input);
        HerdChallengeConfig config;
        config.enabled = root.value("enabled", true);
        config.standstillSeconds = root.value("standstillSeconds", 2.0f);
        config.startingSlimes = root.value("startingSlimes", 1);
        config.slimesPerLevel = root.value("slimesPerLevel", 1);
        config.nextRoundDelay = root.value("nextRoundDelay", 1.0f);
        config.oneHitChallenge = root.value("oneHitChallenge", true);
        config.resultPopupDelay = root.value("resultPopupDelay", 2.0f);
        return apply(config, error);
    } catch (const std::exception &e) { error = e.what(); return false; }
}
bool HerdChallenge::save(const std::filesystem::path &path, std::string &error) const {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::trunc);
    if (!output) { error = "Could not write Herd Challenge settings"; return false; }
    output << json{{"schemaVersion", 1}, {"enabled", config_.enabled},
                   {"standstillSeconds", config_.standstillSeconds},
                   {"startingSlimes", config_.startingSlimes},
                   {"slimesPerLevel", config_.slimesPerLevel},
                   {"nextRoundDelay", config_.nextRoundDelay},
                   {"oneHitChallenge", config_.oneHitChallenge},
                   {"resultPopupDelay", config_.resultPopupDelay}}.dump(4) << '\n';
    return static_cast<bool>(output);
}
void HerdChallenge::reset() {
    state_ = HerdChallengeState::Herding; level_ = 0; stillTime_ = 0;
    requiredHits_ = 0; observedAttack_ = false;
    hitIds_.clear(); resultDelayRemaining_ = 0; spawnRequested_ = config_.enabled;
}
bool HerdChallenge::updateBeforeCombat(float dt, bool moved, bool attacking,
                                      const std::function<bool()> &trigger, int alive) {
    if (!config_.enabled || !config_.oneHitChallenge ||
        state_ != HerdChallengeState::Herding) return false;
    if (attacking) { observedAttack_ = true; return false; }
    stillTime_ = moved ? 0.0f : stillTime_ + std::max(0.0f, dt);
    if (stillTime_ < config_.standstillSeconds) return false;
    stillTime_ = 0.0f;
    if (!trigger()) return false;
    state_ = HerdChallengeState::Swinging; observedAttack_ = true;
    requiredHits_ = alive; hitIds_.clear(); return true;
}
void HerdChallenge::updateAfterCombat(float, bool attacking,
                                      const std::vector<std::uint64_t> &hits) {
    if (!config_.enabled) return;
    if (!config_.oneHitChallenge && state_ == HerdChallengeState::Herding) {
        hitIds_.insert(hits.begin(), hits.end());
        if (requiredHits_ > 0 && static_cast<int>(hitIds_.size()) >= requiredHits_) {
            state_ = HerdChallengeState::LevelComplete;
            resultDelayRemaining_ = config_.resultPopupDelay;
        }
        return;
    }
    if (state_ == HerdChallengeState::Swinging) {
        hitIds_.insert(hits.begin(), hits.end());
        if (observedAttack_ && !attacking) {
            state_ = requiredHits_ > 0 && static_cast<int>(hitIds_.size()) >= requiredHits_
                         ? HerdChallengeState::LevelComplete : HerdChallengeState::Missed;
            resultDelayRemaining_ = config_.resultPopupDelay;
            observedAttack_ = false;
        }
    }
}
void HerdChallenge::beginRound(int monsterCount) {
    requiredHits_ = std::max(0, monsterCount);
    hitIds_.clear(); resultDelayRemaining_ = 0;
}
void HerdChallenge::updateResultDelay(float deltaTime) {
    resultDelayRemaining_ = std::max(0.0f, resultDelayRemaining_ - std::max(0.0f, deltaTime));
}
bool HerdChallenge::consumeSpawnRequest() {
    const bool result = spawnRequested_; spawnRequested_ = false; return result;
}
int HerdChallenge::requestedMonsterCount() const {
    return config_.startingSlimes + level_ * config_.slimesPerLevel;
}
bool HerdChallenge::nextLevel() {
    if (state_ != HerdChallengeState::LevelComplete) return false;
    ++level_; state_ = HerdChallengeState::Herding; stillTime_ = 0;
    requiredHits_ = 0; hitIds_.clear(); spawnRequested_ = true; return true;
}
bool HerdChallenge::retryLevel() {
    if (state_ != HerdChallengeState::Missed) return false;
    level_ = 0; state_ = HerdChallengeState::Herding; stillTime_ = 0;
    requiredHits_ = 0; hitIds_.clear(); spawnRequested_ = true; return true;
}
const char *herdChallengeStateName(HerdChallengeState state) {
    switch (state) {
    case HerdChallengeState::Herding: return "Herding";
    case HerdChallengeState::Swinging: return "Swinging";
    case HerdChallengeState::LevelComplete: return "Level complete";
    case HerdChallengeState::Missed: return "Missed swing";
    }
    return "Unknown";
}
