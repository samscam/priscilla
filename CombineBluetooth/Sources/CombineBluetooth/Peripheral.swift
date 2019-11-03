import Foundation
import CoreBluetooth
import Combine

// PERIPHERAL
public protocol Advertiser: InnerPeripheralProtocol{
    static var advertised: [InnerServiceProtocol.Type] { get }
}

public extension Advertiser where Self:InnerPeripheralProtocol{
    static var advertisedUUIDs: Set<CBUUID> {
        return Set(Self.advertised.map{ $0.uuid })
    }
}


public enum ConnectionState {

    case disconnected(error: Error? = nil)
    case connecting
    case connected
}


open class Peripheral: PeripheralProtocol, ObservableObject {
    public let objectWillChange: ObservableObjectPublisher = ObservableObjectPublisher()
    
    @Published public var state: ConnectionState = .disconnected(error: nil)
    @Published public var name: String
    
    public var advertisementData: AdvertisementData? {
        return self.connection?.advertisementData
    }
    
    weak var connection: Connection?
    
    public let uuid: UUID
    
    public required init(uuid: UUID, name: String, connection: Connection){
        
        self.uuid = uuid
        self.name = name
        self.connection = connection
    }
    
    public init(uuid: UUID, name: String){
        
        self.uuid = uuid
        self.name = name
    }
    
    public func connect(){
        connection?.connect()
    }
    
    public func disconnect(){
        connection?.disconnect()
    }


}

public protocol PeripheralProtocol: ObservableObject, InnerPeripheralProtocol {}
public protocol InnerPeripheralProtocol: class {
    var uuid: UUID { get }
    var name: String { get set }
    var state: ConnectionState { get set }
    init(uuid: UUID, name: String, connection: Connection)
    var objectWillChange: ObservableObjectPublisher { get }
    
}


public extension InnerPeripheralProtocol {
    var serviceUUIDs: [CBUUID]{
        let m = Mirror(reflecting: self)
        return m.children.compactMap{ $0.value as? HasUUID }.map{ $0.uuid }
    }
    
    var services: [InnerServiceProtocol]{
        let m = Mirror(reflecting: self)
        return m.children.compactMap{ $0.value as? InnerServiceProtocol }
    }
    
    var serviceWrappers: [ServiceWrapper]{
        let m = Mirror(reflecting: self)
        return m.children.compactMap { $0.value as? ServiceWrapper}
    }
    
    func serviceWrapper(for cbService: CBService) -> ServiceWrapper? {
        return serviceWrappers.first { (service) -> Bool in
            return service.uuid == cbService.uuid
        }
    }

}
