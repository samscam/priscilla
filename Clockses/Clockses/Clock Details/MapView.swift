//
//  MapView.swift
//  Clockses
//
//  Created by Sam Easterby-Smith on 31/10/2019.
//  Copyright © 2019 Spotlight Kid Ltd. All rights reserved.
//

import SwiftUI
import MapKit

struct MapView: UIViewRepresentable {
    var coordinate: CLLocationCoordinate2D
    
    func makeUIView(context: Context) -> MKMapView {
        MKMapView(frame: .zero)
    }
    
    func updateUIView(_ uiView: MKMapView, context: Context) {
        uiView.isScrollEnabled = false
        uiView.isZoomEnabled = false
        let span = MKCoordinateSpan(latitudeDelta: 0.01, longitudeDelta: 0.01)
       let region = MKCoordinateRegion(center: coordinate, span: span)
        uiView.setRegion(region, animated: true)
    }
}

struct MapView_Previews: PreviewProvider {
    static var previews: some View {
        let coord = CLLocationCoordinate2D(latitude: 34.011286, longitude: -116.166868)
        return MapView(coordinate: coord)
    }
}
