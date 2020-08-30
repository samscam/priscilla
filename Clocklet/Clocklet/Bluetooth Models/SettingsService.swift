//
//  SettingsService.swift
//  Clocklet
//
//  Created by Sam Easterby-Smith on 06/05/2020.
//  Copyright © 2020 Spotlight Kid Ltd. All rights reserved.
//

import Foundation
import CoreBluetooth
import CombineBluetooth
import Combine
import SwiftUI

class SettingsService: ServiceProtocol {
    
    var bag = Set<AnyCancellable>()
    
    required init(){
        subj.throttle(for: 0.05, scheduler: RunLoop.main, latest: true).map{ $0 as Float? }.assign(to: \.brightness, on: self).store(in: &bag)
    }
    
    @Characteristic("9982B160-23EF-42FF-9848-31D9FF21F490") var availableSeparatorAnimations: [String]?
    
    @Characteristic("2371E298-DCE5-4E1C-9CB2-5542213CE81C") var separatorAnimation: String?

    @Characteristic("698D2B57-5B54-48D7-A483-1AB4660FBAF9") var availableTimeStyles: [String]?
    
    @Characteristic("AE35C2DE-7D36-4699-A5CE-A0FA6A0A5483") var timeStyle: String?
    
    @Characteristic("8612F6ED-AA92-45A7-8B46-166F600BC53D") var brightness: Float?
    @Characteristic("A56AE45C-EB82-4182-9B9C-C91F509C91D5") var autoBrightness: Bool?
    
    let subj = PassthroughSubject<Float,Never>()
    
    lazy var selectedBrightness = Binding<Float>(
        get:{ return self.brightness ?? 0.5 },
        set:{ self.subj.send($0) }
    )
    
    lazy var b_autoBrightness = Binding<Bool>(
        get:{ return self.autoBrightness ?? true },
        set:{ self.autoBrightness = $0 }
    )
    
    
    lazy var selectedTimeStyle = Binding<String>(
        get:{ return self.timeStyle ?? "24 Hour" },
        set:{ self.timeStyle = $0 }
    )
    
    lazy var selectedAnimation = Binding<String>(
        get:{ return self.separatorAnimation ?? "Blink"},
        set:{ self.separatorAnimation = $0 }
    )
}

extension SettingsService {
    var timeStyles: [String] { get {
        return self.availableTimeStyles ?? []
    }}
    
    var separatorAnimations: [String] { get {
        return self.availableSeparatorAnimations ?? []
    }}
        
}



enum SeparatorAnimation: String, RawRepresentable, Identifiable, Codable {
    var id: String { return self.rawValue }
    
    case None
    case Blink
    case Fade
}


extension SeparatorAnimation: DataConvertible {


    public init?(data: Data){
        guard let value = String(data: data, encoding: .utf8) else {
            return nil
        }
        self.init(rawValue:value)
    }

    public var data: Data {
        return Data(id.utf8)
    }
}

