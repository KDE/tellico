#!/usr/bin/env ruby
#
# ***************************************************************************
#    copyright            : (C) 2006 by Steve Beattie
#                         : (C) 2008 by Sven Werlen
#    email                : sbeattie@suse.de
#                         : sven@boisdechet.org
# ***************************************************************************
#
# ***************************************************************************
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of version 2 of the GNU General Public License as  *
# *   published by the Free Software Foundation;                            *
# *                                                                         *
# ***************************************************************************

# $Id: boardgamegeek.rb 313 2006-10-02 22:17:11Z steve $

# This program is expected to be invoked from tellico
# (http://periapsis.org/tellico) as an external data source. It provides
# searches for boardgames from the boardgamegeek.com website, via
# boardgamegeek's xmlapi interface
# (http://www.boardgamegeek.com/xmlapi/)
#
# It only allows searches via name; the boardgamegeek xmlapi is not yet
# rich enough to support queries by designer, publisher, category, or
# mechanism. I'd like to add support for querying by boardgamegeek id,
# but that needs additional support in tellico.
#
# Sven Werlen: 03 Feb 2008: script has been extended to retrieve cover
# images (/thumbnail from xmlapi). Images are retrieved from the website
# and base64 is generated on-the-fly.
#
require 'rexml/document'
require 'net/http'
require 'cgi'
require "base64"
include REXML

$my_version = '$Rev: 313 $'

class Game
  attr_writer :year
  attr_writer :description
  attr_writer :cover
  attr_writer :image

  def initialize(name, id)
    @name = name
    @id = id
    @publishers = []
    @designers = []
    @players = []
  end

  def add_publisher(publisher)
    @publishers << publisher
  end

  def add_designer(designer)
    @designers << designer
  end

  def add_players(players)
    @players << players
  end

  def to_s()
    "@name (#@id #@publishers #@year)"
  end

  def toXML()
    element = Element.new 'entry'
    element.add_element Element.new('title').add_text(@name)
    element.add_element Element.new('description').add_text(@description) if @description
    element.add_element Element.new('year').add_text(@year) if @year
    element.add_element Element.new('boardgamegeek-link').add_text("http://www.boardgamegeek/game/#{@id}") if @id
    element.add_element Element.new('bggid').add_text(@id) if @id
    element.add_element Element.new('cover').add_text(@cover) if @cover
  
    if @publishers.length > 0
      pub_elements = Element.new('publishers')
      @publishers.each {|p| pub_elements.add_element Element.new('publisher').add_text(p)}
      element.add_element pub_elements
    end
    if @designers.length > 0
      des_elements = Element.new('designers')
      @designers.each {|d| des_elements.add_element Element.new('designer').add_text(d)}
      element.add_element des_elements
    end
    if @players.length > 0
      players_elements = Element.new('num-players')
      @players.each {|n| players_elements.add_element Element.new('num-player').add_text(n.to_s)}
      element.add_element players_elements
    end
    return element
  end
  
  def image()
    image = Element.new 'image'
    image.add_attribute('format', 'JPEG')
    image.add_attribute('id', @id + ".jpg")
    image.add_text(@image)
    return image
  end
end

def getGameList(query)
  #puts("Query is #{query}")

  search_result = nil
  Net::HTTP.start('www.boardgamegeek.com', 80) do
    |http| search_result = (http.get("/xmlapi/search?search=#{CGI.escape(query)}",
                                    {"User-Agent" => "BoardGameGeek plugin for Tellico #{$my_version}"}).body)
      http.finish
  end
  doc = REXML::Document.new(search_result)

  games = XPath.match(doc, "//game")
  #games.each {|g| puts g.elements['name'].text+g.attributes['gameid']}
  ids = []
  games.each {|g| ids << g.attributes['gameid']}
  return ids
end

def getGameDetails(ids)
  #ids.each {|id| puts id}

  query = "/xmlapi/game/#{ids.join(',')}"
  #puts query
  search_result = nil
  Net::HTTP.start('www.boardgamegeek.com', 80) do |http|
    search_result = http.get(query, {"User-Agent" => "BoardGameGeek plugin for Tellico #{$my_version}"})
    http.finish
  end
  games = []
  case search_result
  when Net::HTTPOK then
    doc = REXML::Document.new(search_result.body)

    games_xml = XPath.match(doc, "//game")
    games_xml.each do |g| 
      if( g.elements['name'] != nil )
        game = Game.new(g.elements['name'].text, g.attributes['gameid'])
        game.year = g.elements['yearpublished'].text
        game.description = g.elements['description'].text
        g.elements.each('publisher'){|p| game.add_publisher p.elements['name'].text}
        g.elements.each('designer'){|d| game.add_designer d.elements['name'].text}
        minp = Integer(g.elements['minplayers'].text)
        maxp = Integer(g.elements['maxplayers'].text)
        minp.upto(maxp) {|n| game.add_players(n)}
        
        # retrieve cover
        coverurl = g.elements['thumbnail'] != nil ? g.elements['thumbnail'].text : nil
        if( coverurl =~ /files.boardgamegeek.com(.*)$/ )
          # puts "downloading... " + $1
          cover = nil
          Net::HTTP.start('files.boardgamegeek.com', 80) do |http| 
            cover = (http.get($1, {"User-Agent" => "BoardGameGeek plugin for Tellico #{$my_version}"}))
          end
          case cover
          when Net::HTTPOK then
            game.cover = g.attributes['gameid'] + ".jpg";
            game.image = Base64.encode64(cover.body);
          end
        else
          # puts "invalid cover: " + coverurl
        end
        games << game
      end
    end
  end
  return games
end

def listToXML(gameList)
  doc = REXML::Document.new
  doc << REXML::DocType.new('tellico PUBLIC', '"-//Robby Stephenson/DTD Tellico V10.0//EN" "http://periapsis.org/tellico/dtd/v10/tellico.dtd"')
  doc << XMLDecl.new
  tellico = Element.new 'tellico'
  tellico.add_attribute('xmlns', 'http://periapsis.org/tellico/')
  tellico.add_attribute('syntaxVersion', '10')
  collection = Element.new 'collection'
  collection.add_attribute('title', 'My Collection')
  collection.add_attribute('type', '13')

  fields = Element.new 'fields'
  field = Element.new 'field'
  field.add_attribute('name', '_default')
  fields.add_element(field)
  field = Element.new 'field'
  field.add_attribute('name', 'bggid')
  field.add_attribute('title', 'BoardGameGeek ID')
  field.add_attribute('category', 'General')
  field.add_attribute('flags', '0')
  field.add_attribute('format', '4')
  field.add_attribute('type', '6')
  field.add_attribute('i18n', 'true')
  fields.add_element(field)
  collection.add_element(fields)

  images = Element.new 'images'

  id = 0
  gameList.each do
    |g| element = g.toXML()
        element.add_attribute('id', id)
        id = id + 1
      collection.add_element(element)
      images.add_element(g.image());
  end
  collection.add_element(images);  
  tellico.add_element(collection)
  doc.add_element(tellico)
  doc.write($stdout, 0)
  puts ""
end

if __FILE__ == $0

  def showUsage
    warn "usage: #{__FILE__} game_query"
    exit 1
  end

  showUsage unless ARGV.length == 1

  idList = getGameList(ARGV.shift)
  if idList
    gameList = getGameDetails(idList)
  end

  listToXML(gameList)
end
