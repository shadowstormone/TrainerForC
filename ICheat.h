#pragma once
class ICheat
{
public:
    virtual void Apply() = 0;
    virtual void Revert() = 0;
    virtual const char* GetName() const = 0;
    virtual bool IsActive() const = 0;
    virtual void Toggle();
    virtual ~ICheat() = default;
};
