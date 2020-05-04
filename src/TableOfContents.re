open Webapi.Dom;

type heading = {
  name: string,
  url: string,
  sub: bool,
  currentPage: option(bool),
};

type data = {
  title: option(string),
  headings: array(heading),
};

type position = {
  y: float,
  url: string,
};

let body = Utils.body;

let footer =
  Document.querySelector(".global-footer", document)->Utils.assertExists;

module Row = {
  [@react.component]
  let make = (~heading: heading, ~current: string) => {
    let onClick =
      React.useCallback0(_ => {
        Element.classList(body)
        |> DomTokenList.remove("show-menu")
        |> Utils.noop1
      });
    let linkClassName =
      React.useMemo2(
        () => {
          let currentPage =
            switch (heading.currentPage) {
            | Some(value) => value
            | None => false
            };

          let className =
            current == heading.url ? "toc__link toc__link--on" : "toc__link";

          currentPage ? {j|$className toc__link--current-page|j} : className;
        },
        (heading, current),
      );

    <li className={heading.sub ? "toc__sub-item" : "toc__item"}>
      <a className=linkClassName href={heading.url} onClick>
        {React.string(heading.name)}
      </a>
    </li>;
  };
};

[@bs.scope "JSON"] [@bs.val] external parseData: string => data = "parse";

let getTop = (rect: Dom.domRect) => Utils.getDomRectValues(rect).top;
let getY = (rect: Dom.domRect) => Utils.getDomRectValues(rect).y;

let calcBoundaryPosition = (heading: Dom.element) => {
  let top = Element.getBoundingClientRect(heading)->getTop;
  let marginTop =
    Window.getComputedStyle(heading, window)
    ->Utils.getComputedStyle("marginTop");

  Window.scrollY(window) +. top -. marginTop -. 60.0;
};

let initTitle: unit => option(string) = () => None;

[@react.component]
let make = () => {
  let (headings, setHeadings) = React.useState(() => [||]);
  let (positions, setPositions) = React.useState(() => [||]);
  let (current, setCurrent) = React.useState(() => "");
  let (title, setTitle) = React.useState(initTitle);
  let (bottom, setBottom) = React.useState(() => "0px");

  let onScroll =
    React.useCallback1(
      () => {
        // first, change the size of the table of contents to align with the footer
        let footerY = Element.getBoundingClientRect(footer)->getY;
        let innerHeight = Window.innerHeight(window)->Js.Int.toFloat;
        let bottom = footerY < innerHeight ? innerHeight -. footerY : 0.0;

        let _ = setBottom(_ => Js.Float.toString(bottom) ++ "px");
        // then, apply style to items of the table of contents based on scroll position
        let item =
          Belt.Array.reverse(positions)
          |> Js.Array.find(item => item.y <= Window.scrollY(window));

        switch (item, Belt.Array.length(positions) > 0) {
        | (Some(current), _) => setCurrent(_ => current.url)
        | (None, true) => setCurrent(_ => positions[0].url)
        | (None, false) => ()
        };
      },
      [|positions|],
    );

  React.useEffect1(
    () => {
      Window.addEventListener("scroll", _ => onScroll(), window);
      onScroll();
      Some(
        () => Window.removeEventListener("scroll", _ => onScroll(), window),
      );
    },
    [|onScroll|],
  );

  React.useEffect0(() => {
    let jsonData =
      Document.getElementById("table-of-contents-data", document)
      ->(
          fun
          | Some(element) => Element.textContent(element)
          | None => ""
        )
      ->parseData;

    let positions =
      jsonData.headings
      ->Js.Array.filter(
          (heading: heading) => Utils.isAnchorLink(heading.url),
          _,
        )
      ->Belt.Array.map(heading => {
          let element = Document.querySelector(heading.url, document);
          {
            y: Utils.assertExists(element)->calcBoundaryPosition,
            url: heading.url,
          };
        });

    setPositions(_ => positions);
    setHeadings(_ => jsonData.headings);
    setTitle(_ => jsonData.title);

    Some(() => ());
  });

  <div className="toc" style={ReactDOMRe.Style.make(~bottom, ())}>
    {switch (title) {
     | Some(value) => <h1 className="toc__title"> {React.string(value)} </h1>
     | None => React.null
     }}
    <ul>
      {headings
       ->Belt.Array.mapWithIndex((_, heading) => {
           <Row heading current key={heading.url} />
         })
       ->React.array}
    </ul>
  </div>;
};
