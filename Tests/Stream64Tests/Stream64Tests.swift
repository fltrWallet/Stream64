//===----------------------------------------------------------------------===//
//
// This source file is part of the Stream64 open source project
//
// Copyright (c) 2022 fltrWallet AG and the Stream64 project authors
// Licensed under Apache License v2.0
//
// See LICENSE.md for license information
//
// SPDX-License-Identifier: Apache-2.0
//
//===----------------------------------------------------------------------===//
import Stream64
import XCTest

final class Stream64Tests: XCTestCase {
    func testWriteReadUnary() {
        let input = (0..<103).map { _ in UInt64(1) } + [ 0 ]
        guard let result = try? Stream64.streamWrite(values: input, p: 1)
        else {
            XCTFail()
            return
        }
        let expected: [UInt8] = [255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 254]
        XCTAssertEqual(result, expected)
        
        let stream64 = Stream64(data: result, count: input.count, p: 1)
        let all: [UInt64] = .init(stream64)
        XCTAssertEqual(input, all)
    }

    func testWriteReadP11() {
        let input: [UInt64] = [ 0, 2047, 0, 2047, ]
        let expected: [UInt8] = [ 0, 31, 252, 0, 127, 240, ]
        
        guard let result = try? Stream64.streamWrite(values: input, p: 11)
        else {
            XCTFail()
            return
        }
        
        XCTAssertEqual(result, expected)
        XCTAssertEqual(input, [UInt64](Stream64(data: result, p: 11)))
    }
    
    func testWriteReadP19() {
        let input: [UInt64] = [0, 524287, 0, 524287, ]
        let expected: [UInt8] = [0, 0, 0x1f, 0xff, 0xfc, 0, 0, 127, 0xff, 0xf0 ]
        
        guard let result = try? Stream64.streamWrite(values: input, p: 19)
        else {
            XCTFail()
            return
        }
        
        XCTAssertEqual(result, expected)
        XCTAssertEqual(input, [UInt64](Stream64(data: result, p: 19)))
    }
    
    func testWriteReadRoundtrip() {
        let input = (0..<100_000).map { _ in UInt64(2047) }
        guard let result = try? Stream64.streamWrite(values: input, p: 11)
        else {
            XCTFail()
            return
        }
        
        let resultCount = input.count / 8 * 11
        XCTAssertEqual(result.count, resultCount)

        let resultData = (0..<resultCount).map { _ in UInt8.max }
        XCTAssertEqual(result, resultData)
        
        let stream = Stream64(data: result, p: 11)
        let all: [UInt64] = .init(stream)
        XCTAssertEqual(all, input)
    }
    
    func testWriteReadRandom() {
        (0..<100).forEach { _ in
            let p = Int.random(in: 1...56)
            let maxValue = Int(pow(Double(2), Double(p))) - 1
            let input = (1 ... .random(in: (1..<20_000))).map { min(UInt64.random(in: 0..<$0), UInt64(maxValue)) }
            
            guard let result = try? Stream64.streamWrite(values: input, p: p)
            else {
                XCTFail()
                return
            }
            let stream64 = p > 7 ? Stream64(data: result, p: p) : Stream64(data: result, count: input.count, p: p)
            
            let all: [UInt64] = Array(stream64)
            XCTAssertEqual(all.count, input.count)
            
            stream64.enumerated().forEach {
                XCTAssertEqual(input[$0.offset], $0.element)
            }
        }
    }
}

