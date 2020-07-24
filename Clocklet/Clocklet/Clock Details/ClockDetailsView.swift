//
//  ClockDetailsView.swift
//  Clocklet
//
//  Created by Sam Easterby-Smith on 10/10/2019.
//  Copyright © 2019 Spotlight Kid Ltd. All rights reserved.
//

import SwiftUI
import Network
import CombineBluetooth


struct ClockDetailsView: View {
    
    @EnvironmentObject var clock: Clock
    
    var body: some View {
        ScrollView{
            VStack(){
                Image(clock.caseColor.imageName)
                    .resizable()
                    .aspectRatio(contentMode: .fit)
                    .frame(width: nil, height: 200, alignment: .center)
                
                if(clock.state == .connected){

                    if(clock.isConfigured == .configured){
                        clock.settingsService.map{ settingsService in
                            ClockSettingsView().environmentObject(settingsService)
                        }
                    }

                    
                    clock.networkService.map{ networkService in
                        NavigationLink(destination: NetworkDetailView().environmentObject(networkService)){
                            NetworkSummaryView().environmentObject(networkService)
                            }.accentColor(nil)
                        
                    }
                    
                    clock.locationService.map{ locationService in
                        LocationSummaryView().environmentObject(locationService)
                    }
                    
                    
                    clock.technicalService.map{ technicalService in
                        clock.deviceInfoService.map{ deviceInfoService in
                            NavigationLink(destination:
                            ClockTechnicalView().environmentObject(technicalService)
                                .environmentObject(deviceInfoService)){
                                ConfigItemView(icon: Image(systemName:"wrench") ,
                                               title: "Technical stuff"){
                                               EmptyView()
                                }
                            }
                        }
                    }
                    
                } else {
                    VStack(alignment:.center){
                        Image(systemName:clock.state.iconSystemName)
                            .resizable()
                            .aspectRatio(contentMode: .fit)
                            .frame(width: 100, height: 100, alignment: /*@START_MENU_TOKEN@*/.center/*@END_MENU_TOKEN@*/)
                            .foregroundColor(clock.state.color)
                            .scaleEffect(clock.state == .connecting ? 0.8 : 1)
                            .animation(Animation.easeInOut(duration: 1).repeatForever())
                        
                        Text(clock.state.description).bold()
                        
                        // Note that if-let works in xcode 12
                        clock.state.lastErrorDescription.map{ lastError in
                            Text(lastError).lineLimit(nil).fixedSize(horizontal: false, vertical: true)
                        }
                        if clock.state == .disconnected() {
                            Spacer()
                            Button("Reconnect") {
                                self.clock.connect()
                            }.accentColor(Color(.systemBackground))
                            .padding()
                            .background(Capsule().fill(Color.accentColor))
                        }
                        
                    }.padding().frame(maxWidth: .infinity)

                }
            }
            .padding()
            .animation(.default)

            
        }.navigationBarTitle( Text(clock.name), displayMode:.automatic)
        .navigationBarItems(trailing: Image(systemName:clock.state.iconSystemName).foregroundColor(clock.state.color))
        .onAppear {
             self.clock.connect()
        }
    }
}

extension ContentSizeCategory{
    static func allCases() -> [ContentSizeCategory]{
        return self.allCases
    }
}

struct ClockDetailsView_Previews: PreviewProvider {
    static let clock: Clock = {
        let clock = Clock("Foop",.bones)
        clock.connect()
        return clock
    }()
//    static let viewModel: ClockDetailsViewModel = {
//        var clock = Clock("Foop",.bones)
//
//        clock.caseColor = .wood
//        clock.networkService?.currentNetwork = CurrentNetwork(status: .connected, connected: true, ssid: "Fishnet", channel: 1, ip: nil, rssi: -20)
////        clock.locationService.currentLocation = CurrentLocation(lat: 53.431808, lng: -2.218080)
//        var viewModel = ClockDetailsViewModel(clock: clock)
//        viewModel.connectionErrorMessage = "This all went wrong today"
//        return viewModel
//    }()
    
    static var previews: some View {

        NavigationView{
            ClockDetailsView().environmentObject(clock)
        }
        
    }
    //        ClockDetailsView(clock: )
    //        let clock = ClockModel(id: UUID(), serial: 5, name: "Clocklet #291", caseColor: .wood)
    //
    //
    //        let currentNetwork = CurrentNetwork(status: 4, connected: true, ssid: "Broccoli", channel: 5, ip: IPv4Address("129.12.41.5"), rssi: -2)
    //        let networkService = NetworkService()
    //        networkService.currentNetwork = currentNetwork
    //        networkService.availableNetworks = [
    //        AvailableNetwork(ssid: "Broccoli", enctype: .open, rssi: -20, channel: 4, bssid:"SOMETHING"),
    //        AvailableNetwork(ssid: "My Wifi is Nice", enctype: .open, rssi: -20, channel: 4, bssid:"mywifi")
    //        ]
    //        let connection = ClockConnection(clock: clock, networkService: networkService)
    //        return ClockDetailsView(clockConnection: connection)
    
}

extension ConnectionState{
    var iconSystemName: String {
        switch self {
            case .connected: return "bolt.fill"
            case .connecting: return "bolt"
            case .disconnected: return "bolt.slash.fill"
        }
    }
    
    var color: Color {
        switch self {
            case .connected: return .green
            case .connecting: return .orange
            case .disconnected: return .red
        }
    }
}

extension ConnectionState: CustomStringConvertible {

    public var description: String {
        switch self {
            case .connected: return "Connected"
            case .connecting: return "Connecting"
            case .disconnected: return "Disconnected"
        }
    }
    
    public var lastErrorDescription: String? {
        switch self {
        case .connected, .connecting:
            return nil
        case .disconnected(let error):
            return error?.localizedDescription
        }
    }
}
