#include "sim/Battle.h"

#include <algorithm>
#include <cmath>

namespace sim {

namespace {

glm::vec3 flat(glm::vec3 v) {
    v.y = 0.0f;
    return v;
}

glm::vec3 safeNormalize(const glm::vec3& v, const glm::vec3& fallback) {
    const float l = glm::length(v);
    return l > 1e-5f ? v / l : fallback;
}

const glm::vec3 kUp(0.0f, 1.0f, 0.0f);
const glm::vec3 kBodyOffset(0.0f, 1.0f, 0.0f);   // centro del cuerpo respecto a los pies

}  // namespace

Battle::Battle(const BattleConfig& cfg) : cfg_(cfg), rng_(cfg.seed) {
    reset(cfg.seed);
}

void Battle::reset(std::uint64_t seed) {
    cfg_.seed = seed;
    rng_ = Rng(seed);
    time_ = 0.0f;
    outcome_ = Outcome::Running;
    timedOut_ = false;
    projectiles_.clear();
    stats_ = {};

    const float x = cfg_.rules.arenaRadius * 0.6f;
    for (int i = 0; i < 2; ++i) {
        Fighter f;
        f.id = i;
        f.pos = glm::vec3(i == 0 ? -x : x, 0.0f, 0.0f);
        f.facing = glm::vec3(i == 0 ? 1.0f : -1.0f, 0.0f, 0.0f);
        f.hp = cfg_.rules.maxHp;
        f.mana = cfg_.rules.maxMana;
        fighters_[i] = f;
    }
}

float Battle::actionDuration(Action a) const {
    const Rules& r = cfg_.rules;
    switch (a) {
        case Action::CastMagic: return r.magicWindup + r.magicRecovery;
        case Action::Dodge:     return r.dodgeDuration;
        case Action::Block:     return r.blockDuration;
        case Action::Idle:      return 0.0f;
    }
    return 0.0f;
}

float Battle::actionProgress(int i) const {
    const Fighter& f = fighters_[i];
    if (!f.busy()) return 0.0f;
    const float d = actionDuration(f.current);
    return d > 0.0f ? std::min(1.0f, f.actionElapsed / d) : 0.0f;
}

bool Battle::isDodging(int i) const {
    return fighters_[i].current == Action::Dodge;
}

bool Battle::isShielding(int i) const {
    return fighters_[i].current == Action::Block;
}

glm::vec3 Battle::shieldCenter(int i) const {
    const Fighter& f = fighters_[i];
    return f.pos + f.facing * cfg_.rules.shieldDistance + kBodyOffset;
}

ActionMask Battle::legalActions(int i) const {
    ActionMask m;   // solo Idle
    const Fighter& f = fighters_[i];
    if (outcome_ != Outcome::Running || !f.alive() || f.busy()) return m;

    const Rules& r = cfg_.rules;
    if (cfg_.allowed.has(Action::CastMagic) && f.cdMagic <= 0.0f && f.mana >= r.magicCost) m.set(Action::CastMagic, true);
    if (cfg_.allowed.has(Action::Dodge) && f.cdDodge <= 0.0f && f.mana >= r.dodgeCost) m.set(Action::Dodge, true);
    if (cfg_.allowed.has(Action::Block) && f.cdBlock <= 0.0f && f.mana >= r.blockCost) m.set(Action::Block, true);
    return m;
}

const Projectile* Battle::nearestIncoming(int i, float* outTime) const {
    const Fighter& f = fighters_[i];
    const float hitRadius = cfg_.rules.fighterRadius + cfg_.rules.magicRadius + 0.6f;
    const glm::vec3 body = f.pos + kBodyOffset;

    const Projectile* best = nullptr;
    float bestT = 1e9f;
    for (const Projectile& p : projectiles_) {
        if (p.owner == i) continue;
        const float speed = glm::length(p.vel);
        if (speed < 1e-4f) continue;
        const glm::vec3 dir = p.vel / speed;
        const glm::vec3 r = body - p.pos;
        const float along = glm::dot(r, dir);            // lo que le falta hasta el punto mas cercano
        if (along <= 0.0f) continue;                     // ya ha pasado
        const float lateral = glm::length(r - dir * along);
        if (lateral > hitRadius) continue;               // no le apunta
        const float t = along / speed;
        if (t < bestT) {
            bestT = t;
            best = &p;
        }
    }
    if (outTime) *outTime = bestT;
    return best;
}

Observation Battle::observe(int i) const {
    const Fighter& f = fighters_[i];
    const Fighter& o = fighters_[1 - i];
    const Rules& r = cfg_.rules;

    Observation ob;
    ob.dt = kDt;
    ob.selfHp = f.hp / r.maxHp;
    ob.selfMana = f.mana / r.maxMana;
    ob.opponentHp = o.hp / r.maxHp;
    ob.opponentMana = o.mana / r.maxMana;
    ob.distance = glm::length(flat(o.pos - f.pos));
    ob.cdMagic = f.cdMagic;
    ob.cdDodge = f.cdDodge;
    ob.cdBlock = f.cdBlock;
    ob.busy = f.busy();
    ob.timeLeft = std::max(0.0f, cfg_.maxDuration - time_);

    float t = 0.0f;
    if (const Projectile* p = nearestIncoming(i, &t)) {
        ob.incoming = true;
        ob.incomingTime = t;
        ob.incomingDistance = glm::length(p->pos - (f.pos + kBodyOffset));
    }
    return ob;
}

glm::vec3 Battle::chooseDodgeDir(const Fighter& f) {
    const glm::vec3 right = glm::cross(kUp, f.facing);
    float t = 0.0f;
    if (const Projectile* p = nearestIncoming(f.id, &t)) {
        const float side = glm::dot(p->pos - f.pos, right);
        if (std::fabs(side) > 0.05f) return side > 0.0f ? -right : right;   // se aparta de la trayectoria
    }
    return rng_.chance(0.5f) ? right : -right;
}

void Battle::startAction(int i, Action a) {
    if (a == Action::Idle) return;
    if (!legalActions(i).has(a)) return;   // accion ilegal => se ignora

    const Rules& r = cfg_.rules;
    Fighter& f = fighters_[i];
    f.current = a;
    f.actionElapsed = 0.0f;
    f.projectileSpawned = false;

    switch (a) {
        case Action::CastMagic:
            f.mana -= r.magicCost;
            f.cdMagic = r.magicCooldown;
            break;
        case Action::Dodge:
            f.mana -= r.dodgeCost;
            f.cdDodge = r.dodgeCooldown;
            f.dodgeDir = chooseDodgeDir(f);
            break;
        case Action::Block:
            f.mana -= r.blockCost;
            f.cdBlock = r.blockCooldown;
            break;
        case Action::Idle:
            break;
    }
}

void Battle::spawnProjectile(const Fighter& f) {
    const Rules& r = cfg_.rules;
    const Fighter& o = fighters_[1 - f.id];

    Projectile p;
    p.owner = f.id;
    p.pos = f.pos + f.facing * 0.9f + glm::vec3(0.0f, 1.2f, 0.0f);
    glm::vec3 dir = safeNormalize((o.pos + kBodyOffset) - p.pos, f.facing);

    // Pequena imprecision de punteria (rotacion alrededor de Y)
    const float a = rng_.range(-r.magicSpread, r.magicSpread);
    const float c = std::cos(a), s = std::sin(a);
    dir = glm::vec3(dir.x * c + dir.z * s, dir.y, -dir.x * s + dir.z * c);

    p.vel = dir * r.magicSpeed;
    p.life = r.magicLife;
    projectiles_.push_back(p);
    stats_[f.id].casts++;
}

void Battle::updateFighter(int i) {
    const Rules& r = cfg_.rules;
    Fighter& f = fighters_[i];
    const Fighter& o = fighters_[1 - i];

    f.cdMagic = std::max(0.0f, f.cdMagic - kDt);
    f.cdDodge = std::max(0.0f, f.cdDodge - kDt);
    f.cdBlock = std::max(0.0f, f.cdBlock - kDt);
    f.hitTimer = std::max(0.0f, f.hitTimer - kDt);
    f.mana = std::min(r.maxMana, f.mana + r.manaRegen * kDt);
    f.facing = safeNormalize(flat(o.pos - f.pos), f.facing);

    if (!f.alive()) {
        f.deadTime += kDt;
        f.current = Action::Idle;
        return;
    }

    if (f.busy()) {
        f.actionElapsed += kDt;
        switch (f.current) {
            case Action::CastMagic:
                if (!f.projectileSpawned && f.actionElapsed >= r.magicWindup) {
                    spawnProjectile(f);
                    f.projectileSpawned = true;
                }
                break;
            case Action::Dodge:
                f.pos += f.dodgeDir * r.dodgeSpeed * kDt;
                break;
            default:
                break;
        }
        if (f.actionElapsed >= actionDuration(f.current)) {
            f.current = Action::Idle;
            f.actionElapsed = 0.0f;
        }
    }

    // Mantener al combatiente dentro de la arena
    const float maxR = r.arenaRadius - r.fighterRadius;
    const float len = glm::length(flat(f.pos));
    if (len > maxR) {
        f.pos.x *= maxR / len;
        f.pos.z *= maxR / len;
    }
}

void Battle::updateProjectiles() {
    const Rules& r = cfg_.rules;
    for (std::size_t k = 0; k < projectiles_.size();) {
        Projectile& p = projectiles_[k];
        p.pos += p.vel * kDt;
        p.life -= kDt;

        bool remove = p.life <= 0.0f;
        if (!remove) {
            const int target = 1 - p.owner;
            Fighter& t = fighters_[target];
            if (t.alive()) {
                if (isShielding(target) && glm::distance(p.pos, shieldCenter(target)) < r.shieldRadius + r.magicRadius) {
                    stats_[target].blocked++;
                    remove = true;
                }
                if (!remove && glm::distance(p.pos, t.pos + kBodyOffset) < r.fighterRadius + r.magicRadius) {
                    if (isDodging(target)) {   // invulnerable: el proyectil lo atraviesa
                        if (!p.dodgeCounted) {
                            p.dodgeCounted = true;
                            stats_[target].dodged++;
                        }
                    } else {
                        t.hp -= r.magicDamage;
                        t.hitTimer = 0.25f;
                        stats_[p.owner].hits++;
                        stats_[p.owner].damageDealt += r.magicDamage;
                        remove = true;
                    }
                }
            }
            if (glm::length(flat(p.pos)) > r.arenaRadius * 1.6f) remove = true;
        }

        if (remove)
            projectiles_.erase(projectiles_.begin() + static_cast<std::ptrdiff_t>(k));
        else
            ++k;
    }
}

void Battle::evaluateOutcome() {
    const bool d0 = !fighters_[0].alive();
    const bool d1 = !fighters_[1].alive();
    if (d0 || d1) {
        outcome_ = (d0 && d1) ? Outcome::Draw : (d0 ? Outcome::Fighter1Wins : Outcome::Fighter0Wins);
        return;
    }
    if (time_ >= cfg_.maxDuration) {
        timedOut_ = true;
        const float h0 = fighters_[0].hp, h1 = fighters_[1].hp;
        outcome_ = h0 > h1 ? Outcome::Fighter0Wins : (h1 > h0 ? Outcome::Fighter1Wins : Outcome::Draw);
    }
}

void Battle::step(Action a0, Action a1) {
    if (outcome_ != Outcome::Running) return;

    startAction(0, a0);
    startAction(1, a1);
    updateFighter(0);
    updateFighter(1);
    updateProjectiles();
    time_ += kDt;
    evaluateOutcome();
}

}  // namespace sim
