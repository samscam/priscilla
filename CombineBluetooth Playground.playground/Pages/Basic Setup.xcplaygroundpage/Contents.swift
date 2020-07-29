import CoreBluetooth
import Combine
import PlaygroundSupport
import CombineBluetooth

PlaygroundPage.current.needsIndefiniteExecution = true

var bag = Set<AnyCancellable>()

/// Create a model for the peripheral, composed of a bunch of services
class ThingWithInfo: Peripheral, AdvertisementMatcher {

    static var advertisedServiceUUIDs: [String] = ["180A"]

    @Service("180A") var deviceInfoService: DeviceInfoService?

}

/// Create models for each Service
class DeviceInfoService: ServiceProtocol {
    required init(){}

    @Characteristic("2A29") var manufacturerName: String?
    @Characteristic("2A24") var model: String?
    @Characteristic("2A25") var serialNumber: String?
    @Characteristic("2A26") var firmwareVersion: String?

}


let central = Central()
var connectionPub = central.discoverConnections(for: ThingWithInfo.self)

connectionPub.sink { (connections) in
    print(connections)
}.store(in: &bag)


//
//
//class NetworkService: ServiceProtocol {
//    static var uuid: CBUUID = CBUUID(string:"68D924A1-C1B2-497B-AC16-FD1D98EDB41F")
//
//    @Characteristic(CBUUID(string:"BEB5483E-36E1-4688-B7F5-EA07361B26A8")) var availableNetwork: Int? = 32
//    @Characteristic(CBUUID(string:"CEB5483E-36E1-4688-B7F5-EA07361B26A8")) var wibbles: String = "Hello"
//}
//
//class InfoService: ServiceProtocol{
//    static var uuid: CBUUID = CBUUID(string:"CCB5483E-36E1-4688-B7F5-EA07361B26A8")
//
//    @Characteristic(CBUUID(string:"EEEEE83E-36E1-4688-B7F5-EA07361B26A8")) var hardwareRevision: Int? = nil
//    @Characteristic(CBUUID(string:"FFFFF83E-36E1-4688-B7F5-EA07361B26A8")) var firmwareVersion: String? = nil
//}
//
//let netService = NetworkService()
//netService.characteristicUUIDs
//netService.availableNetwork
//netService.availableNetwork = 5
//netService.$availableNetwork.uuid
//
//netService.wibbles = "Really"
//
//netService.$wibbles.uuid
//
//// Peripheral examples
//
//
//
//let clock = Clock(uuid: UUID(), name:"Big Ben")
//clock.services
//clock.serviceUUIDs
//clock.rawServices
//Clock.advertisedUUIDs
//
//
//
////PlaygroundPage.current.needsIndefiniteExecution = true
//
//let central = Central()
//var connectionPub = central.discoverConnections(for: Clock.self)
//
//var connectionSubscription: AnyCancellable? = connectionPub.sink(receiveCompletion: { (comp) in
//    print("Completion")
//}) { (connections) in
//    print("**** I got connections ****")
//
//    for connection in connections {
//        print(" \(connection.cbPeripheral.identifier.shortString) \(connection.cbPeripheral.name ?? "No name") \(connection.peripheral.debugDescription)")
//    }
//}
//
//DispatchQueue.main.asyncAfter(deadline: .now() + 30) {
//    connectionSubscription = nil
//}
//
