"""
Python Interface for Nico USB HID sensors
NOTE: Need admin priviledges to access the USB
"""

# Modules
import usb.core
import usb.util

# Constants
OUT_DATA_CODE_READ_VOLTAGE = 0x81
OUT_DATA_CODE_CALIBRATE = 0x82
OUT_DATA_CODE_GET_CAL_DATA = 0x83
OUT_DATA_CODE_GET_SW_VERSION = 0x84

IN_DATA_A2D_DATA = 0x41
IN_DATA_CAL_DATA = 0x42
IN_DATA_SW_VERSION_DATA = 0x43

SIZE_IN_DATA = 61
SIZE_OUT_DATA = 61
SIZE_MAX_WRITE = 59
SIZE_MAX_READ = 59

# Pretty Print
def pretty_print(task, msg):
    print("%s\t%s" % (task, msg))

# PyUS Function
class NicoUSB:

    def __init__(self):
    
        # Initialize Device Connection
        config = {
            'product' : 0x8468,
            'vendor' : 0x10c4,
            'interface' : 0x0,
            'manufacturer' : 'SLAB',
            'read_timeout' : 1,
            'write_timeout' : 1000
            }
        self.config = config
        self.dev = None
        self.mode = None
        all_devs = usb.core.find('10c4:8468') # Finding UDEV Devices
        for d in all_devs:
            try:
                if d.manufacturer == config['manufacturer']:
                    self.dev = d
            except usb.USBError as e:
                print str(e)
            except Exception as e:
                print str(e)
                
        if self.dev is None: 
            raise ValueError('[ERR 100] Device not found, check permissions.')
        else:
        
            # Print configuration
            print self.dev
            
            # Get the Active Configuration
            try:
                pretty_print('WARNING', 'Getting udev config')
                self.cfg = self.dev.get_active_configuration()
                interface_number = self.cfg[(0,0)].bInterfaceNumber
                pretty_print('WARNING', 'Interface #%d' % interface_number)
            except Exception as e:
                pretty_print('ERR 101', str(e))
    
            # Alternate Settings
            try:
                pretty_print('WARNING', 'Getting alternate interface setting')
                alternate_setting = usb.control.get_interface(self.dev, interface_number)
            except usb.core.USBError as e:
                pretty_print('ERR 102', str(e))
            except Exception as e:
                pretty_print('ERR 103', str(e))
    
            # Interface
            try:
                pretty_print('WARNING', 'Finding descriptor of interface')
                intf = usb.util.find_descriptor(self.cfg)
            except Exception as e:
                pretty_print('ERR 200', str(e))
    
            # Endpoint In
            try:
                pretty_print('WARNING', 'Finding EP In descriptor')
                self.ep_in = usb.util.find_descriptor(
                    intf,
                    custom_match = \
                    lambda e: \
                        usb.util.endpoint_direction(e.bEndpointAddress) == \
                        usb.util.ENDPOINT_IN
                )
                assert self.ep_in is not None
            except Exception as e:
                pretty_print('ERR 201', 'Failed to set EP In')
    
            # Endpoint Out
            try:
                pretty_print('WARNING', 'Finding EP Out descriptor')
                self.ep_out = usb.util.find_descriptor(
                    intf,
                    custom_match = \
                    lambda e: \
                        usb.util.endpoint_direction(e.bEndpointAddress) == \
                        usb.util.ENDPOINT_OUT
                )
                assert self.ep_out is not None
            except Exception as e:
                pretty_print('ERR 202', 'Failed to set EP Out')

            # Kernel Driver
            try:
                interface = self.config['interface']
                if self.dev is None:
                    raise ValueError('Device not found')
                elif self.dev.is_kernel_driver_active(interface) is True:
                    pretty_print('WARNING', 'Detaching kernel driver')
                    self.dev.detach_kernel_driver(interface) # We need to detach kernel driver
                else:
                    raise ValueError('Kernel driver inactive!')
            except Exception as e:
                pretty_print('ERR 301', str(e))
    
            # Set the active configuration. With no arguments, the first will be the active one
            try:
                pretty_print('WARNING', 'Setting configuration')
                self.dev.set_configuration()
            except Exception as e:
                pretty_print('ERR 302', str(e))

    # Get Voltage
    def get_voltage(self):
        if not self.mode == OUT_DATA_CODE_READ_VOLTAGE:
            self.write_usb(OUT_DATA_CODE_READ_VOLTAGE)
            self.mode = OUT_DATA_CODE_READ_VOLTAGE
        
        data = self.read_usb()
        if data is not None:
            pretty_print('WARNING', 'The following data IS NOT parsed')
            try:
                print data.tolist()
                print data.tostring()
                A2DSum = [data[4], data[3], data[2], data[1]] # invert it for some reason or other
                print A2DSum
                A2DNumReads = [data[6], data[5]] # invert this too
                print A2DNumReads
                A2DAverage = (max(A2DSum) + (min(A2DSum) / 2)) / min(A2DNumReads)
                raw_voltage = -1.0 * A2DAverage / 32000 # or 32768.0 * 1.024;
                print raw_voltage
                return raw_voltage #!TODO parse data into meaningful result
            except Exception as e:
                pretty_print('ERR 999', str(e))
            
            ## Calculate raw voltage
            # A2DAverage = (A2DSum.longvalue + (A2DNumReads.shortvalue/2 )) / A2DNumReads.shortvalue;
            # m_RawVoltage = A2DAverage / 32000 # or 32768.0 * 1.024;
            # m_RawVoltage *= -1 ;  
            # join_int = lambda nums: int(''.join(str(i) for i in nums)) #! FIXME
            # res = join_int(data)

    def get_cal_data(self):
        self.write_usb(OUT_DATA_CODE_GET_CAL_DATA)
        return self.read_usb()
    
    def get_version(self):
        self.write_usb(OUT_DATA_CODE_GET_SW_VERSION)
        return self.read_usb()
    
    # Write request command
    def write_usb(self, cmd_byte):
        try:
            self.dev.write(self.ep_out.bEndpointAddress, str(cmd_byte), self.config['write_timeout'])
        except usb.core.USBError as e:
            pretty_print('ERR 400', str(e))
            return
        except Exception as e:
            pretty_print('ERR 401', str(e))
            return
        
    # Read response
    def read_usb(self):
        try:
            data = self.dev.read(self.ep_in.bEndpointAddress, SIZE_MAX_READ, self.config['read_timeout'])
            if data is not None:
                #data = data.tolist()
                return data
            else:
                pretty_print('ERR 500', 'Data is not a voltage reading')
        except AttributeError as e:
            #pretty_print('ERR 501', str(e))
            return
        except usb.core.USBError as e:
            #pretty_print('ERR 502', str(e))
            return
        except Exception as e:
            #pretty_print('ERR 503', str(e))
            return
        
    # Reset Connection
    def reset(self):
        self.dev.reset()

if __name__ == '__main__':
    root = NicoUSB()
    while True:
        try:
            v = root.get_voltage()
            if v: pretty_print('READ OK', 'Voltage: %s' % v)
        except KeyboardInterrupt:
            break
