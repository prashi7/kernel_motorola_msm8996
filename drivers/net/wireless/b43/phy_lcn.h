#ifndef B43_PHY_LCN_H_
#define B43_PHY_LCN_H_

#include "phy_common.h"


#define B43_PHY_LCN_AFE_CTL1			B43_PHY_OFDM(0x03B)
#define B43_PHY_LCN_AFE_CTL2			B43_PHY_OFDM(0x03C)
#define B43_PHY_LCN_RF_CTL1			B43_PHY_OFDM(0x04C)
#define B43_PHY_LCN_RF_CTL2			B43_PHY_OFDM(0x04D)
#define B43_PHY_LCN_RF_CTL3			B43_PHY_OFDM(0x0B0)
#define B43_PHY_LCN_RF_CTL4			B43_PHY_OFDM(0x0B1)
#define B43_PHY_LCN_RF_CTL5			B43_PHY_OFDM(0x0B7)
#define B43_PHY_LCN_RF_CTL6			B43_PHY_OFDM(0x0F9)
#define B43_PHY_LCN_RF_CTL7			B43_PHY_OFDM(0x0FA)


struct b43_phy_lcn {
};


struct b43_phy_operations;
extern const struct b43_phy_operations b43_phyops_lcn;

#endif /* B43_PHY_LCN_H_ */
