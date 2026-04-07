#ifndef FC_HEAD_H
#define FC_HEAD_H

#include <QObject>
#include <QList>
#include <algorithm>
#include <cstdint>

#include "FCRange.h"
#include "FCI2CDevice.h"
#include "FCNozzle.h"

class FCHead
    : public FCI2CDevice
{
Q_OBJECT
Q_DISABLE_COPY_MOVE(FCHead)
public:
    explicit FCHead(FCI2CBus *bus, uint8_t address = 0, const QString &name = QString(),
                    const FCNozzleList &nozzles = FCNozzleList() , QObject *parent = nullptr)
        : FCI2CDevice(bus, address, name, parent),
          _nozzles{nozzles}
    {}

    ~FCHead() override = default;

    [[nodiscard]] inline int count() const noexcept { return _nozzles.size(); }

    void selectColor(const QColor &color);

    [[nodiscard]] QString securityCode(int timeoutMs) override;

private:
    uint8_t calculateFeeder(const QColor &color);

    bool removeAllFeeders();

    bool submitFeeder(uint8_t number);

    [[nodiscard]] inline const FCNozzle& nozzle(int index) const noexcept
    {
        return _nozzles[FCRange<int>(0, _nozzles.size() - 1).clamped(index)];
    }

    [[nodiscard]] inline bool isValidNozzleIndex(int index) const noexcept
    {
        return FCRange<int>(0, _nozzles.size() - 1).contains(index);
    }

    FCNozzleList _nozzles;
};

#endif // FC_HEAD_H
