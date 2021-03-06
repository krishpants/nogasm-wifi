#include "../../include/UIMenu.h"
#include "../../include/ButtplugRegistry.h"

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

int scanTime = 30; //In seconds
BLEScan* pBLEScan = nullptr;
bool scanning = false;
UIMenu *globalMenuPtr = nullptr;

static void startScan(UIMenu *);
static void stopScan(UIMenu *);

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  UIMenu *menuPtr = nullptr;

public:
  MyAdvertisedDeviceCallbacks(UIMenu *menu) {
    this->menuPtr = menu;
  }

  void onResult(BLEAdvertisedDevice advertisedDevice) {
    log_i("Advertised Device: %s", advertisedDevice.toString().c_str());
    BLEAdvertisedDevice *device = new BLEAdvertisedDevice(advertisedDevice);

    if (advertisedDevice.haveName()) {
      this->menuPtr->addItem(advertisedDevice.getName().c_str(), [](UIMenu *menu, void *device) {
        stopScan(menu);
        log_i("Clicked Device: %s", ((BLEAdvertisedDevice *) device)->getAddress().toString().c_str());
        Buttplug.connect((BLEAdvertisedDevice*) device);
      }, device);

      this->menuPtr->render();
    }
  }
};

static void freeMenuPtrs(UIMenu *menu) {
  UIMenuItem *ptr = menu->firstItem();

  while (ptr != nullptr) {
    if (ptr->arg != nullptr) {
      BLEAddress *addr = (BLEAddress*) ptr->arg;
      delete addr;
      ptr->arg = nullptr;
    }
    ptr = ptr->next;
  }
}

static void addScanItem(UIMenu *menu) {
  menu->removeItem(0);

  if (!scanning) {
    menu->addItemAt(0, "Scan for Devices", &startScan);
  } else {
    menu->addItemAt(0, "Stop Scanning", &stopScan);
  }
}

static void startScan(UIMenu *menu) {
  scanning = true;
  pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory
  freeMenuPtrs(menu);
  menu->initialize();
  menu->render();

  pBLEScan->start(scanTime, [](BLEScanResults foundDevices) {
    scanning = false;
    addScanItem(globalMenuPtr);
    globalMenuPtr->render();
  }, false);
}

static void stopScan(UIMenu *menu) {
  scanning = false;
  pBLEScan->stop();
  addScanItem(menu);
  menu->render();
}

static void menuOpen(UIMenu *menu) {
  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks(menu));
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // less or equal setInterval value

  // Default start scan:
  startScan(menu);
}

static void menuClose(UIMenu *menu) {
  pBLEScan->stop();
  pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory
  scanning = false;
  freeMenuPtrs(menu);
  pBLEScan = nullptr;
}

static void buildMenu(UIMenu *menu) {
  globalMenuPtr = menu;
  menu->onOpen(&menuOpen);
  menu->onClose(&menuClose);

  addScanItem(menu);
}

UIMenu BluetoothScanMenu("Bluetooth Pair", &buildMenu);