#ifndef BLEMANAGER_H_
#define BLEMANAGER_H_

#ifdef __cplusplus
extern "C" {
#endif

#define BLE_SV_CHAR1  0xAB01
#define BLE_SV_CHAR2  0xAB02

class BLEManager{
    private:
        BLEManager();
        ~BLEManager();
    
        BLEManager(const BLEManager&) = delete;
        BLEManager& operator = (const BLEManager&) = delete;
    
        static BLEManager s_instance;
       
        CmdManager *cmd;
        ClockManager *clk;
        
        void processedCommandResponseCallback(const DeviceCommand& rsp);

    public:
        static BLEManager* getInstance();   
        
        static void bleprph_on_reset(int reason);
        static void bleprph_on_sync(void);
        static int bleprph_gap_event(struct ble_gap_event *event, void *arg);

        static void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg);
        static int gatt_svc_access(uint16_t conn_handle, uint16_t attr_handle,struct ble_gatt_access_ctxt *ctxt,void *arg);
            
        void bleprph_advertise(void);
        int gatt_svr_init(void);
        void initialize(void);
};

#ifdef __cplusplus
}
#endif

#endif /* !BLEMANAGER_H_ */
