#include "Damage.h"

Damage::Damage(
    float amount,
    ElementType element,
    EffectType effect,
    float effectValue,
    float duration)
{
    m_amount = amount;
    m_element = element;
    m_effect = effect;
    m_effectValue = effectValue;
    m_duration = duration;
}

float Damage::getAmount() const
{
    return m_amount;
}

ElementType Damage::getElement() const
{
    return m_element;
}

EffectType Damage::getEffect() const
{
    return m_effect;
}

float Damage::getEffectValue() const
{
    return m_effectValue;
}

float Damage::getDuration() const
{
    return m_duration;
}