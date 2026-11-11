#ifndef USB4_STAGES_H
#define USB4_STAGES_H

static void usb4_pll_wait(void) {
  static __xdata uint16_t i;
  for (i = 0; !((X(LPHY0_BASE + LPHY_STATUS) & LPHY_PLL_LOCKED) &&
              (X(LPHY1_BASE + LPHY_STATUS) & LPHY_PLL_LOCKED)) &&
              i < USB4_SPIN; i++);
  if (i == USB4_SPIN) usb4_to_x++;
  else usb4_to_x = 0;
}

static void usb4_lanecfg(uint16_t a) __reentrant {
  X(a) = (X(a) & 0x80) | 0x40 | ((X(a + 0x23) >> 3) & 0x07);
}

static void usb4_sb_ctl(uint8_t hdr) __reentrant {
  bank1_wr(B1_SB_TX_HDR, hdr);
  bank1_mod(B1_SB_TX_FRAME_LEN, 0x7F, 0x03);
  bank1_mod(B1_SB_TX_CTRL, 0x00, SB_TX_HDR_ONLY);
  bank1_mod(B1_LANE_MUX_CTL, 0x01, 0x00);
  bank1_mod(0x2804, 0x02, 0x00);
  usb4_sb_kick();
  usb4_timer_once(USB4_TMR_P1, 0x0B);
  bank1_mod(B1_SB_TX_CTRL, SB_TX_HDR_ONLY, 0x00);
}

static void usb4_rxpll_reset(void) {
  x_mod(REG_ADDR(REG_CPU_CTRL_CC37), 0x00, CPU_CTRL_CC37_RXPLL_MODE);
  REG_PHY_RXPLL_RESET = PHY_RXPLL_ASSERT;
  usb4_timer_once(USB4_TMR_P1, 0x14);
  REG_PHY_RXPLL_RESET = 0x00;
  usb4_timer_once(USB4_TMR_P2, 0x28);
  x_mod(REG_ADDR(REG_CPU_CTRL_CC37), CPU_CTRL_CC37_RXPLL_MODE, 0x00);
}

static void usb4_ph4_poll(void) {
  static __xdata uint8_t n;
  for (n = 0; n < 2; n++) {
    bank1_wr(B1_SB_TX_HDR, 0x61);
    bank1_wr(B1_SB_TX_BUF, 0x09);
    bank1_mod(B1_SB_TX_BUF, 0x00, 0x04);
    bank1_wr(B1_SB_TX_WLEN, 0x23);
    bank1_mod(B1_SB_TX_FRAME_LEN, 0x7F, 0x08);
    bank1_mod(B1_LANE_MUX_CTL, 0x01, 0x00);
    bank1_mod(0x2804, 0x02, 0x00);
    usb4_sb_kick();
    usb4_timer_once(USB4_TMR_P1, 0x0B);
    bank1_mod(B1_SB_TX_CTRL, SB_TX_HDR_ONLY, 0x00);
    bank1_wr(B1_SB_TX_BUF, 0x00);
    bank1_wr(B1_SB_TX_WLEN, 0x00);
    usb4_timer_once(USB4_TMR_P2, 0xC8);
  }
}

static void usb4_pcie_start(void) {
  x_mod(REG_ADDR(REG_PHY_TIMER_CTRL_E764), 0x00, 0x08);
  x_mod(REG_ADDR(REG_PHY_TIMER_CTRL_E764), 0x04, 0x00);
  x_mod(REG_ADDR(REG_PHY_TIMER_CTRL_E764), 0x01, 0x00);
  x_mod(REG_ADDR(REG_PHY_TIMER_CTRL_E764), 0x00, 0x02);
  usb4_timer_once(USB4_TMR_P1, 0x07CF);
  x_mod(REG_ADDR(REG_PHY_TIMER_CTRL_E764), 0x08, 0x00);
  x_mod(REG_ADDR(REG_PHY_TIMER_CTRL_E764), 0x04, 0x00);
  x_mod(REG_ADDR(REG_PHY_TIMER_CTRL_E764), 0x01, 0x00);
  x_mod(REG_ADDR(REG_PHY_TIMER_CTRL_E764), 0x02, 0x00);
  x_mod(REG_ADDR(REG_PHY_CFG_C657), 0x00, PHY_C657_SLOT);
  x_mod(REG_ADDR(REG_HDDPC_CTRL), 0x00, HDDPC_ENABLE);
}

static void usb4_ph4_start(void) {
  x_mod(REG_ADDR(REG_LINK_STATUS_E716), 0x00, LINK_STATUS_E716_MASK);
  usb4_timer_once(USB4_TMR_P2, 0x28);
  x_mod(REG_ADDR(REG_LINK_STATUS_E716), LINK_STATUS_E716_MASK, 0x00);
  x_mod(REG_ADDR(REG_LINK_STATUS_E716), 0x00, LINK_STATUS_E716_MASK);
  x_mod(REG_ADDR(REG_CPU_CTRL_CA81), 0x01, 0x00);
  x_mod(REG_ADDR(REG_CPU_MODE_NEXT), 0x80, 0x60);
  x_mod(REG_ADDR(REG_CPU_MODE_NEXT), 0xE0, 0x00);
  x_mod(REG_ADDR(REG_PCIE_LANE_CTRL_C659), PCIE_LANE_EN, 0x00);
  x_mod(REG_ADDR(REG_PCIE_CTRL_B402), PCIE_CTRL_B402_BIT0, 0x00);
  usb4_pcie_lane_reset();
  x_mod(REG_ADDR(REG_CPU_EXT_CTRL), TIMER_DIV_IRQ_MASK, 0x00);
  REG_CPU_EXT_STATUS = TIMER_CSR_CLEAR;
  REG_CPU_EXT_STATUS = TIMER_CSR_EXPIRED;
  x_mod(REG_ADDR(REG_TUNNEL_CTRL_B403), 0x01, 0x00);
  bank1_mod(B1_DSP_CFG + PCI_LNKCTL2, 0x0B, 0x04);
  x_mod(REG_ADDR(REG_TUNNEL_LINK_STATUS), PCIE_LINK_STATE_MASK, 0x00);
  usb4_pcie_lane_reset();
  usb4_pcie_start();
  usb4_sb_ctl(0x82);
  usb4_sb_ctl(0xA2);
  x_mod(REG_ADDR(REG_CPU_CTRL_CC37), 0x00, CPU_CTRL_CC37_RXPLL_MODE);
  bank1_mod(B1_LINK_GEN_CTRL, 0x10, 0x00);
  bank1_mod(B1_LINK_GEN_CTRL, 0x20, 0x00);
  bank1_mod(B1_LINK_GEN_CTRL, 0x00, 0x10);
  bank1_mod(B1_LINK_GEN_CTRL, 0x00, 0x20);
  usb4_timer_once(USB4_TMR_P2, 0xC8);
  x_mod(LPHY0_BASE + LPHY_PLL_CTRL, 0xC0, 0x00);
  x_mod(LPHY1_BASE + LPHY_PLL_CTRL, 0xC0, 0x00);
  REG_PHY_CTRL_C20F = PHY_LANE_PLL_RESTART;
  usb4_timer_once(USB4_TMR_P1, 0x14);
  REG_PHY_CTRL_C20F = 0x00;
  usb4_pll_wait();
  x_mod(LPHY0_BASE + LPHY_PLL_CTRL, 0xC0, 0x00);
  usb4_lanecfg(LPHY0_BASE + LPHY_LANE_CFG);
  x_mod(LPHY1_BASE + LPHY_PLL_CTRL, 0xC0, 0x00);
  usb4_lanecfg(LPHY1_BASE + LPHY_LANE_CFG);
  x_mod(LPHY0_BASE + LPHY_PLL_CTRL, 0x40, 0x80);
  x_mod(LPHY1_BASE + LPHY_PLL_CTRL, 0x40, 0x80);
  usb4_rxpll_reset();
}

static void usb4_ph4_end(void) {
  x_mod(REG_ADDR(REG_CPU_CTRL_CA60), 0x00, 0x08);
  x_mod(REG_ADDR(REG_TIMER_CCB0_DIV), 0x02, 0x05);
  REG_TIMER_CCB0_THRESHOLD_HI = 0x00;
  REG_TIMER_CCB0_THRESHOLD_LO = 0xC8;
  bank1_wr(0x134D, 0x04);
  bank1_wr(0x1334, 0x02);
  bank1_wr(0x1335, 0x02);
  bank1_mod(0x2805, 0x00, 0x01);
  bank1_mod(0x2805, 0x02, 0x00);
  bank1_mod(0x2805, 0x00, 0x04);
  bank1_mod(0x2804, 0x00, 0x80);
  bank1_mod(0x1285, 0xC0, 0x30);
  bank1_wr(B1_LANE0_TRIG, LANE_TRIG_OS1);
  bank1_mod(B1_LANE_OS1, 0x00, 0x01);
  bank1_wr(B1_LANE1_TRIG, LANE_TRIG_OS1);
  bank1_mod(B1_LANE_OS1, 0x00, 0x02);
  usb4_cce_restart();
}

#endif
